#!/usr/bin/env python3
"""
DTO Generator for C++ from OpenAPI specification
Generates DTO classes with serialization/deserialization to/from JSON
"""

import argparse
import yaml
import json
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
from jinja2 import Environment, FileSystemLoader, Template
import re


class DTOGenerator:
    """Generates C++ DTO classes from OpenAPI specification"""

    # C++ type mappings
    TYPE_MAPPINGS = {
        'integer': 'int64_t',
        'string': 'std::string',
        'boolean': 'bool',
        'number': 'double',
        'array': 'std::vector',
        'object': 'nlohmann::json',
    }

    # OpenAPI format to C++ type mappings
    FORMAT_MAPPINGS = {
        'int32': 'int32_t',
        'int64': 'int64_t',
        'float': 'float',
        'double': 'double',
        'date-time': 'std::chrono::system_clock::time_point',
        'date': 'std::chrono::system_clock::time_point',
        'binary': 'std::vector<uint8_t>',
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
            # Extract type name from reference
            ref = schema['$ref']
            type_name = ref.split('/')[-1]
            return type_name

        schema_type = schema.get('type', 'string')
        schema_format = schema.get('format', '')

        # Check format first
        if schema_format in self.FORMAT_MAPPINGS:
            return self.FORMAT_MAPPINGS[schema_format]

        # Handle arrays
        if schema_type == 'array':
            items = schema.get('items', {})
            item_type = self.get_cpp_type(items)
            return f"std::vector<{item_type}>"

        # Handle objects
        if schema_type == 'object':
            if 'properties' in schema:
                # This is a nested object - we'll treat it as a DTO
                # For now, return nlohmann::json as fallback
                return 'nlohmann::json'
            return 'nlohmann::json'

        # Handle enums
        if 'enum' in schema:
            return 'std::string'

        # Basic types
        return self.TYPE_MAPPINGS.get(schema_type, 'std::string')

    def get_array_item_type(self, schema: Dict[str, Any]) -> str:
        """Get the item type for an array"""
        items = schema.get('items', {})
        return self.get_cpp_type(items)

    def is_required(self, schema_name: str, property_name: str) -> bool:
        """Check if a property is required"""
        schemas = self.spec.get('components', {}).get('schemas', {})
        schema = schemas.get(schema_name, {})
        required = schema.get('required', [])
        return property_name in required

    def get_default_value(self, schema: Dict[str, Any]) -> Optional[str]:
        """Get default value for a property"""
        if 'default' in schema:
            default = schema['default']
            if isinstance(default, str):
                return f'"{default}"'
            return str(default)
        return None

    def get_enum_values(self, schema: Dict[str, Any]) -> List[str]:
        """Get enum values if present"""
        if 'enum' in schema:
            return [str(v) for v in schema['enum']]
        return []

    def to_snake_case(self, name: str) -> str:
        """Convert camelCase to snake_case"""
        # Insert underscore before capital letters
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        # Handle consecutive capitals (e.g., ID -> _id)
        s2 = re.sub('([a-z0-9])([A-Z]+)', r'\1_\2', s1)
        return s2.lower()

    def to_camel_case(self, name: str) -> str:
        """Convert snake_case to lowerCamelCase (keeps original case)"""
        # This method should preserve the original field name from OpenAPI
        # since OpenAPI typically uses camelCase already
        return name

    def to_pascal_case(self, name: str) -> str:
        """Convert to PascalCase for setter methods"""
        # If it's already camelCase, just capitalize first letter
        if name and not name[0].isupper():
            return name[0].upper() + name[1:]
        return name

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
            # Skip writeOnly fields (like password)
            if prop_schema.get('writeOnly'):
                continue

            # Determine if field is required
            is_required = prop_name in required

            # Get C++ type
            cpp_type = self.get_cpp_type(prop_schema, prop_name)

            # For arrays, get item type
            array_item_type = ""
            if prop_schema.get('type') == 'array':
                array_item_type = self.get_array_item_type(prop_schema)

            # Get default value
            default = self.get_default_value(prop_schema)

            # Get enum values
            enum_values = self.get_enum_values(prop_schema)

            # Check if this is a constant value (for API version etc.)
            const_value = None
            if prop_name == 'tokenType' and prop_schema.get('default') == 'Bearer':
                const_value = 'Bearer'

            # Convert name to different cases
            name_snake = self.to_snake_case(prop_name)
            # For camelCase and PascalCase, we keep the original name from OpenAPI
            # because it's already in the correct format (e.g., "projectId")
            name_camel = prop_name  # Keep original camelCase
            name_pascal = prop_name[0].upper() + prop_name[1:] if prop_name else prop_name

            field = {
                'name': prop_name,
                'name_snake': name_snake,
                'name_camel': name_camel,
                'name_pascal': name_pascal,
                'cpp_type': cpp_type,
                'array_item_type': array_item_type,
                'required': is_required,
                'default': default,
                'description': prop_schema.get('description', ''),
                'enum_values': enum_values,
                'const_value': const_value,
            }
            fields.append(field)

        return {
            'name': name,
            'name_snake': self.to_snake_case(name),
            'description': schema.get('description', f'{name} DTO'),
            'fields': fields,
        }

    def collect_dependencies(self, dto: Dict[str, Any]) -> List[str]:
        """Collect dependencies for a DTO"""
        dependencies = set()

        for field in dto['fields']:
            cpp_type = field['cpp_type']

            # Check if this is a reference to another DTO
            if cpp_type in self.dto_names:
                dependencies.add(cpp_type)

            # Check vector of DTOs
            if cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]  # Remove 'std::vector<' and '>'
                if inner_type in self.dto_names:
                    dependencies.add(inner_type)

        return list(dependencies)

    def collect_includes(self, dto: Dict[str, Any]) -> List[str]:
        """Collect includes for a DTO"""
        includes = set()
        includes.add('#include <nlohmann/json.hpp>')
        includes.add('#include <string>')
        includes.add('#include <vector>')
        includes.add('#include <unordered_map>')
        includes.add('#include <memory>')
        includes.add('#include <chrono>')

        # Add includes for dependencies
        for field in dto['fields']:
            cpp_type = field['cpp_type']

            # Add include for the DTO itself if it's a dependency
            if cpp_type in self.dto_names:
                includes.add(f'#include "{self.to_snake_case(cpp_type)}.h"')

            # Add include for vector items
            if cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]
                if inner_type in self.dto_names:
                    includes.add(f'#include "{self.to_snake_case(inner_type)}.h"')

        return sorted(list(includes))

    def generate_dto_header(self, dto: Dict[str, Any], template: Template) -> str:
        """Generate header file for a DTO"""
        includes = self.collect_includes(dto)
        dependencies = self.collect_dependencies(dto)

        return template.render(
            dto=dto,
            includes=includes,
            dependencies=dependencies,
            generated_at=datetime.now().isoformat(),
        )

    def generate_dto_impl(self, dto: Dict[str, Any], template: Template) -> str:
        """Generate implementation file for a DTO"""
        return template.render(
            dto=dto,
            generated_at=datetime.now().isoformat(),
        )

    def generate_common_header(self, template: Template) -> str:
        """Generate common header with forward declarations"""
        includes = [
            '#include <memory>',
            '#include <nlohmann/json.hpp>',
        ]

        return template.render(
            dto_names=self.dto_names,
            includes=includes,
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

        # Load templates
        env = Environment(
            loader=FileSystemLoader(str(self.templates_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
        )

        header_template = env.get_template('dto.h.j2')
        impl_template = env.get_template('dto.cpp.j2')
        common_template = env.get_template('dto_common.h.j2')

        # Generate common header
        common_header_content = self.generate_common_header(common_template)
        self.write_file(self.output_dir / 'dto_common.h', common_header_content)

        # Generate DTOs
        for dto in dtos:
            # Generate header
            header_content = self.generate_dto_header(dto, header_template)
            header_file = self.output_dir / f'{self.to_snake_case(dto["name"])}.h'
            self.write_file(header_file, header_content)

            # Generate implementation
            impl_content = self.generate_dto_impl(dto, impl_template)
            impl_file = self.output_dir / f'{self.to_snake_case(dto["name"])}.cpp'
            self.write_file(impl_file, impl_content)

        print("\n✅ DTO generation completed!")


def main():
    parser = argparse.ArgumentParser(
        description='Generate C++ DTO classes from OpenAPI specification'
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
        help='Output directory for generated files'
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
    generator = DTOGenerator(args.openapi, args.output, args.templates)
    generator.run()

    return 0


if __name__ == '__main__':
    exit(main())
