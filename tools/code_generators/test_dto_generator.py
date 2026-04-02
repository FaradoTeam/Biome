#!/usr/bin/env python3
"""
DTO Test Generator for C++ from OpenAPI specification
Generates unit tests for DTO classes using Boost.Test
"""

import argparse
import yaml
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
from jinja2 import Environment, FileSystemLoader, Template
import re


class DTOTestGenerator:
    """Generates C++ unit tests for DTO classes from OpenAPI specification using Boost.Test"""

    # Test data generators for different types
    TEST_DATA_GENERATORS = {
        'int64_t': '42',
        'int32_t': '42',
        'double': '3.14',
        'float': '3.14f',
        'bool': 'true',
        'std::string': '"test_value"',
        'std::chrono::system_clock::time_point': 'std::chrono::system_clock::now()',
        'nlohmann::json': 'nlohmann::json::object()',
    }

    def __init__(self, openapi_file: Path, output_dir: Path, templates_dir: Path):
        self.openapi_file = openapi_file
        self.output_dir = output_dir
        self.templates_dir = templates_dir
        self.spec = None
        self.dto_names = []

    def load_spec(self) -> None:
        """Load OpenAPI specification from YAML file"""
        with open(self.openapi_file, 'r', encoding='utf-8') as f:
            self.spec = yaml.safe_load(f)

    def get_cpp_type(self, schema: Dict[str, Any], name: str = "") -> str:
        """Convert OpenAPI type to C++ type"""
        if '$ref' in schema:
            ref = schema['$ref']
            type_name = ref.split('/')[-1]
            return type_name

        schema_type = schema.get('type', 'string')
        schema_format = schema.get('format', '')

        if schema_format in ['int32', 'int64']:
            return 'int64_t' if schema_format == 'int64' else 'int32_t'
        if schema_format == 'float':
            return 'float'
        if schema_format == 'double':
            return 'double'
        if schema_format in ['date-time', 'date']:
            return 'std::chrono::system_clock::time_point'

        if schema_type == 'array':
            items = schema.get('items', {})
            item_type = self.get_cpp_type(items)
            return f"std::vector<{item_type}>"

        if schema_type == 'object':
            return 'nlohmann::json'

        if schema_type == 'integer':
            return 'int64_t'
        if schema_type == 'boolean':
            return 'bool'
        if schema_type == 'string':
            return 'std::string'
        if schema_type == 'number':
            return 'double'

        return 'std::string'

    def get_test_value(self, cpp_type: str, field_name: str = "") -> str:
        """Generate test value for a given C++ type"""
        # Handle DTO types
        if cpp_type in self.dto_names:
            return f"{cpp_type}()"

        # Handle vectors
        if cpp_type.startswith('std::vector<'):
            return "{}"

        # Handle basic types
        if cpp_type == 'bool':
            return 'true'
        if cpp_type in ['int64_t', 'int32_t']:
            return '42'
        if cpp_type in ['double', 'float']:
            return '3.14'
        if cpp_type == 'std::string':
            return '"test_value"'
        if cpp_type == 'std::chrono::system_clock::time_point':
            return 'std::chrono::system_clock::now()'

        return '{}'

    def get_json_for_field(self, field: Dict[str, Any]) -> str:
        """Generate JSON string for a field"""
        field_name = field['name_snake']
        cpp_type = field['cpp_type']

        if cpp_type == 'std::string':
            return f'{{"{field_name}", "test_{field_name}"}}'
        elif cpp_type in ['int64_t', 'int32_t']:
            return f'{{"{field_name}", 42}}'
        elif cpp_type == 'bool':
            return f'{{"{field_name}", true}}'
        elif cpp_type in ['double', 'float']:
            return f'{{"{field_name}", 3.14}}'
        elif cpp_type == 'std::chrono::system_clock::time_point':
            return f'{{"{field_name}", 1640995200}}'  # 2022-01-01 00:00:00
        elif cpp_type.startswith('std::vector<'):
            return f'{{"{field_name}", nlohmann::json::array({{"item1", "item2"}})}}'
        elif cpp_type in self.dto_names:
            return f'{{"{field_name}", {cpp_type}().toJson()}}'
        else:
            return f'{{"{field_name}", "test_value"}}'

    def to_snake_case(self, name: str) -> str:
        """Convert camelCase to snake_case"""
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z]+)', r'\1_\2', s1)
        return s2.lower()

    def collect_dtos(self) -> List[Dict[str, Any]]:
        """Collect all DTOs from OpenAPI schemas"""
        schemas = self.spec.get('components', {}).get('schemas', {})
        dtos = []

        for name, schema in schemas.items():
            if schema.get('type') == 'object' and 'properties' in schema:
                dto = self.process_dto(name, schema)
                dtos.append(dto)
                self.dto_names.append(name)

        return dtos

    def process_dto(self, name: str, schema: Dict[str, Any]) -> Dict[str, Any]:
        """Process a single DTO schema"""
        properties = schema.get('properties', {})
        required = schema.get('required', [])

        fields = []
        for prop_name, prop_schema in properties.items():
            # Skip writeOnly fields
            if prop_schema.get('writeOnly'):
                continue

            cpp_type = self.get_cpp_type(prop_schema, prop_name)

            # For arrays, get item type
            array_item_type = ""
            if prop_schema.get('type') == 'array':
                items = prop_schema.get('items', {})
                array_item_type = self.get_cpp_type(items)

            field = {
                'name': prop_name,
                'name_snake': self.to_snake_case(prop_name),
                'name_camel': prop_name,
                'name_pascal': prop_name[0].upper() + prop_name[1:] if prop_name else prop_name,
                'cpp_type': cpp_type,
                'array_item_type': array_item_type,
                'required': prop_name in required,
                'description': prop_schema.get('description', ''),
            }
            fields.append(field)

        return {
            'name': name,
            'name_snake': self.to_snake_case(name),
            'description': schema.get('description', f'{name} DTO'),
            'fields': fields,
        }

    def collect_includes(self, dto: Dict[str, Any]) -> List[str]:
        """Collect includes for test file"""
        includes = set()
        # Don't define BOOST_TEST_MODULE here
        includes.add('#include <boost/test/unit_test.hpp>')
        includes.add('#include <nlohmann/json.hpp>')
        includes.add('#include <chrono>')
        includes.add('#include <sstream>')
        includes.add('#include <iomanip>')

        # Include the DTO header
        includes.add(f'#include "../../common/dto/{dto["name_snake"]}.h"')

        # Include print helpers
        includes.add(f'#include "../../common/dto/dto_print_helpers.h"')

        # Include dependencies
        for field in dto['fields']:
            cpp_type = field['cpp_type']
            if cpp_type in self.dto_names:
                includes.add(f'#include "../../common/dto/{self.to_snake_case(cpp_type)}.h"')
            elif cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]
                if inner_type in self.dto_names:
                    includes.add(f'#include "../../common/dto/{self.to_snake_case(inner_type)}.h"')

        return sorted(list(includes))

    def generate_test_file(self, dto: Dict[str, Any], template: Template) -> str:
        """Generate test file for a DTO"""
        includes = self.collect_includes(dto)

        # Generate test data
        test_data = {}
        for field in dto['fields']:
            if not field['required']:
                test_data[field['name_camel']] = self.get_test_value(field['cpp_type'], field['name'])

        # Generate JSON for serialization test
        json_parts = []
        for field in dto['fields']:
            json_parts.append(f'    {self.get_json_for_field(field)}')

        json_object = ",\n".join(json_parts)

        # Generate field validation checks
        field_checks = []
        for field in dto['fields']:
            if field['required']:
                if field['cpp_type'] == 'std::string':
                    field_checks.append(f'    BOOST_TEST(!dto.{field["name_camel"]}().empty());')
                elif field['cpp_type'].startswith('std::vector<'):
                    field_checks.append(f'    BOOST_TEST(!dto.{field["name_camel"]}().empty());')
                else:
                    field_checks.append(f'    BOOST_TEST(dto.{field["name_camel"]}() != {self.get_test_value(field["cpp_type"], field["name"])});')

        return template.render(
            dto=dto,
            includes=includes,
            json_object=json_object,
            field_checks=field_checks,
            generated_at=datetime.now().isoformat(),
        )

    def write_file(self, filepath: Path, content: str) -> None:
        """Write content to file"""
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Generated: {filepath}")

    def run(self) -> None:
        """Run the generator"""
        print("Loading OpenAPI specification...")
        self.load_spec()

        print("Collecting DTOs...")
        dtos = self.collect_dtos()
        print(f"Found {len(dtos)} DTOs")

        # Load templates with custom filters
        env = Environment(
            loader=FileSystemLoader(str(self.templates_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
        )

        # Add custom filter for length
        env.filters['length'] = lambda x: len(x)

        test_template = env.get_template('dto_test.cpp.j2')

        # Generate tests for each DTO
        for dto in dtos:
            test_content = self.generate_test_file(dto, test_template)
            test_file = self.output_dir / f'test_{dto["name_snake"]}.cpp'
            self.write_file(test_file, test_content)

        # Generate test suite runner
        self.generate_test_runner(dtos)

        print("\n✅ DTO test generation completed!")

    def generate_test_runner(self, dtos: List[Dict[str, Any]]) -> None:
        """Generate main test runner file"""
        runner_content = f"""// Auto-generated main test file for DTO tests
// Generated: {datetime.now().isoformat()}
// DO NOT EDIT MANUALLY

#define BOOST_TEST_MODULE FaradoDtoTests
#include <boost/test/unit_test.hpp>

// The test cases are in separate files
// This file just defines the test module
"""
        runner_file = self.output_dir / 'test_main.cpp'
        self.write_file(runner_file, runner_content)


def main():
    parser = argparse.ArgumentParser(
        description='Generate C++ unit tests for DTO classes using Boost.Test'
    )
    parser.add_argument(
        '--openapi',
        required=True,
        type=Path,
        help='Path to OpenAPI specification file (YAML)'
    )
    parser.add_argument(
        '--output',
        required=True,
        type=Path,
        help='Output directory for generated test files'
    )
    parser.add_argument(
        '--templates',
        required=True,
        type=Path,
        help='Directory containing Jinja2 templates'
    )

    args = parser.parse_args()

    # Validate inputs
    if not args.openapi.exists():
        print(f"Error: OpenAPI file not found: {args.openapi}")
        return 1

    if not args.templates.exists():
        print(f"Error: Templates directory not found: {args.templates}")
        return 1

    # Run generator
    generator = DTOTestGenerator(args.openapi, args.output, args.templates)
    generator.run()

    return 0


if __name__ == '__main__':
    exit(main())
