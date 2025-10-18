import argparse
import os
from pathlib import Path
from dataclasses import dataclass

from cxxheaderparser.simple import (
    parse_string,
    ParsedData,
    NamespaceScope,
    Variable
)


@dataclass(frozen=True)
class Arguments:
    scan_dirs: list[Path]
    output_dir: Path
    output_cmake_file: Path
    cmake_var_name: str


@dataclass(frozen=True)
class Property:
    """
    struct/class의 Property 정보
    """
    type_name: str
    name: str
    metadata: dict[str, str | bool]  # key=value 또는 flag 형식의 메타데이터


@dataclass(frozen=True)
class ReflectionData:
    """
    struct/class의 리플렉션 정보
    """
    namespace: str | None
    type_name: str
    properties: list[Property]


def parse_cxx_header(header_file_path: Path) -> ParsedData | None:
    with open(header_file_path, "r", encoding="utf-8") as f:
        header_content = f.read()

    # SE_REFLECTABLE() 매크로가 존재하는지 확인
    if "SE_REFLECTABLE()" not in header_content:
        return None

    # utf-8-sig를 제거한 후 파싱
    parsed_data = parse_string(header_content.replace("\ufeff", ""))

    # 파싱된 데이터에서 SE_REFLECTABLE() 토큰이 실제로 존재하는지 확인
    if not has_se_reflectable_token(parsed_data):
        return None

    return parsed_data


def has_se_reflectable_token(parsed_data: ParsedData) -> bool:
    """
    파싱된 데이터에서 SE_REFLECTABLE() 매크로가 토큰으로 존재하는지 확인합니다.
    주석 처리된 매크로는 토큰에 포함되지 않으므로 이 함수는 None을 반환합니다.
    """
    def check_scope(scope: NamespaceScope) -> bool:
        # 현재 스코프의 모든 변수에서 확인
        for variable in scope.variables:
            if hasattr(variable, 'value') and hasattr(variable.value, 'tokens'):
                for token in variable.value.tokens:
                    if token.value == "SE_REFLECTABLE":
                        return True

        # 하위 네임스페이스 확인
        for child_scope in scope.namespaces.values():
            if check_scope(child_scope):
                return True

        return False

    return check_scope(parsed_data.namespace)


def flatten_namespace(ns: NamespaceScope) -> list[tuple[str | None, NamespaceScope]]:
    type InnerType = tuple[str | None, NamespaceScope]

    def flatten_recursive(inner: InnerType, out_scopes: list[InnerType]):
        out_scopes.append(inner)
        for name, child in inner[1].namespaces.items():
            flatten_recursive((f"{inner[0]}::{name}" if inner[0] else name, child), out_scopes)

    scopes = []
    flatten_recursive((None, ns), scopes)
    return scopes


def parse_metadata(tokens: list) -> dict[str, str | bool]:
    """
    SE_PROPERTY() 내부의 메타데이터 파싱
    형식: key=value, flag, key=value, ...
    """
    metadata: dict[str, str | bool] = {}

    i = 0
    while i < len(tokens):
        # 공백 토큰 스킵
        if tokens[i].value.strip() == "":
            i += 1
            continue

        # key=value 형식 찾기
        if (i + 2 < len(tokens) and
            tokens[i + 1].value == "="):
            key = tokens[i].value
            value = tokens[i + 2].value
            metadata[key] = value
            i += 3

            # 쉼표 스킵
            if i < len(tokens) and tokens[i].value == ",":
                i += 1
        else:
            # flag 형식 (값이 없는 경우)
            key = tokens[i].value
            if key != ",":
                metadata[key] = True
            i += 1

            # 쉼표 스킵
            if i < len(tokens) and tokens[i].value == ",":
                i += 1

    return metadata


def reflect_type(ns_name: str | None, class_def: Variable) -> ReflectionData:
    assert len(class_def.name.segments) == 1, "이게 두개 일수가 있나?"
    type_name = class_def.name.segments[0].name

    properties: list[Property] = []
    tokens = class_def.value.tokens

    i = 0
    while i < len(tokens):
        # SE_PROPERTY 패턴 찾기
        if (i + 1 < len(tokens) and
            tokens[i].value == "SE_PROPERTY" and
            tokens[i + 1].value == "("):

            # SE_PROPERTY( ... ) 내부의 메타데이터 추출
            j = i + 2
            paren_depth = 1
            metadata_tokens = []

            while j < len(tokens) and paren_depth > 0:
                if tokens[j].value == "(":
                    paren_depth += 1
                    metadata_tokens.append(tokens[j])
                elif tokens[j].value == ")":
                    paren_depth -= 1
                    if paren_depth > 0:
                        metadata_tokens.append(tokens[j])
                else:
                    metadata_tokens.append(tokens[j])
                j += 1

            # 메타데이터 파싱
            metadata = parse_metadata(metadata_tokens)

            # SE_PROPERTY() 이후 타입과 이름 찾기
            type_parts = []

            # 타입 수집 (변수명 직전까지)
            while j < len(tokens):
                if tokens[j].value == "=":
                    # 기본값이 있는 경우, 타입 수집 완료
                    break
                elif tokens[j].value == ";":
                    # 기본값이 없는 경우, 타입 수집 완료
                    break
                type_parts.append(tokens[j].value)
                j += 1

            if type_parts:
                # 마지막 토큰이 변수명
                var_name = type_parts[-1]
                var_type = "".join(type_parts[:-1])

                properties.append(Property(
                    type_name=var_type,
                    name=var_name,
                    metadata=metadata
                ))

            i = j
        else:
            i += 1

    return ReflectionData(
        namespace=ns_name,
        type_name=type_name,
        properties=properties,
    )


def generate_reflection_cpp(reflect_datas: list[ReflectionData], header_file_path: Path) -> str:
    """
    리플렉션 데이터들을 바탕으로 .gen.cpp 파일 내용을 생성합니다.
    """

    # 헤더 include
    lines = [
        f"// THIS FILE IS AUTO-GENERATED BY {Path(__file__).name}. DO NOT EDIT!",
        f"#include \"{header_file_path}\"",
        "#include \"SimpleEngine/Reflection/Reflect.h\"\n",
    ]

    # 각 리플렉션 데이터에 대해 코드 생성
    for i, reflect_data in enumerate(reflect_datas):
        if i > 0:
            lines.append("")

        # 네임스페이스 시작
        if reflect_data.namespace:
            lines.append(f"namespace {reflect_data.namespace}")
            lines.append("{")

        lines.append(f"SE_BEGIN_REFLECT({reflect_data.type_name})")
        for prop in reflect_data.properties:
            lines.append(f"    SE_REFLECT_PROPERTY({prop.name})")
        lines.append(f"SE_END_REFLECT({reflect_data.type_name})")

        # 네임스페이스 종료
        if reflect_data.namespace:
            lines.append("}")

    return "\n".join(lines)


def process_single_header(header_file_path: Path, output_dir_path: Path) -> Path | None:
    parsed_data = parse_cxx_header(header_file_path)
    if parsed_data is None:
        return None

    scopes = flatten_namespace(parsed_data.namespace)
    reflect_datas = [reflect_type(ns_name, class_def) for ns_name, ns in scopes for class_def in ns.variables]

    if not reflect_datas:
        return None

    # 헤더 파일명을 기반으로 .gen.cpp 파일명 생성
    header_stem = header_file_path.stem
    output_filename = f"{header_stem}.gen.cpp"
    output_filepath = output_dir_path / output_filename

    # .gen.cpp 파일 생성
    gen_cpp_content = generate_reflection_cpp(reflect_datas, header_file_path)

    should_write = True
    if os.path.exists(output_filepath):
        with open(output_filepath, "r", encoding="utf-8") as f_old:
            if f_old.read() == gen_cpp_content:
                should_write = False

    if should_write:
        print(f"Generating reflection for: {header_file_path} -> {output_filepath}")
        with open(output_filepath, "w", encoding="utf-8") as f:
            f.write(gen_cpp_content)

    return output_filepath


def main(args: Arguments):
    all_header_files = []
    for scan_dir in args.scan_dirs:
        all_header_files.extend(list(scan_dir.rglob("*.h")))

    # 출력 디렉토리 생성
    args.output_dir.mkdir(parents=True, exist_ok=True)

    # 모든 헤더 파일 처리 및 생성된 .cpp 파일 경로 수집
    generated_cpp_files: list[Path] = []
    for header_path in all_header_files:
        generated_file = process_single_header(header_path, args.output_dir)
        if generated_file:
            generated_cpp_files.append(generated_file)

    for existing_file in args.output_dir.rglob("*.gen.cpp"):
        if existing_file not in generated_cpp_files:
            print(f"Removing stale reflection file: {existing_file}")
            os.remove(existing_file)

    with open(args.output_cmake_file, "w", encoding="utf-8") as f:
        f.write(f"# This file is auto-generated by {Path(__file__).name}. DO NOT EDIT!\n")
        if not generated_cpp_files:
            f.write(f"set({args.cmake_var_name} \"\")\n")
        else:
            cmake_paths = [f"    \"{p.as_posix()}\"" for p in generated_cpp_files]
            f.write(f"set({args.cmake_var_name}\n")
            f.write("\n".join(cmake_paths))
            f.write("\n)\n")

    print(f"Generated {len(generated_cpp_files)} reflection source(s) for {args.cmake_var_name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--scan-dirs", required=True, type=Path, nargs='+')
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--output-cmake-file", required=True, type=Path)
    parser.add_argument("--cmake-var-name", required=True, type=str)

    parse_args = parser.parse_args()
    main(Arguments(
        parse_args.scan_dirs,
        parse_args.output_dir,
        parse_args.output_cmake_file,
        parse_args.cmake_var_name,
    ))
