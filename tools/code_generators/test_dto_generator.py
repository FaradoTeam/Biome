#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Генератор юнит-тестов для DTO классов на C++ из OpenAPI спецификации.
Создаёт тесты с использованием Boost.Test.
"""

import argparse
import yaml
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any
from jinja2 import Environment, FileSystemLoader, Template
import re


class DTOTestGenerator:
    """
    Генерирует C++ юнит-тесты для DTO классов с использованием Boost.Test.

    Атрибуты:
        openapi_file: Путь к файлу OpenAPI спецификации
        output_dir: Директория для сохранения тестов
        templates_dir: Директория с шаблонами Jinja2
        spec: Загруженная OpenAPI спецификация
        dto_names: Список имён DTO
    """

    def __init__(self, openapi_file: Path, output_dir: Path, templates_dir: Path):
        """
        Инициализирует генератор тестов DTO.

        Аргументы:
            openapi_file: Путь к OpenAPI YAML файлу
            output_dir: Директория для вывода тестов
            templates_dir: Директория с Jinja2 шаблонами
        """
        self.openapi_file = openapi_file
        self.output_dir = output_dir
        self.templates_dir = templates_dir
        self.spec = None
        self.dto_names = []

    def load_spec(self) -> None:
        """Загружает OpenAPI спецификацию из YAML файла."""
        with open(self.openapi_file, 'r', encoding='utf-8') as f:
            self.spec = yaml.safe_load(f)

    def get_cpp_type(self, schema: Dict[str, Any], name: str = "") -> str:
        """
        Преобразует OpenAPI тип в C++ тип для тестов.

        Аргументы:
            schema: Схема поля
            name: Имя поля

        Возвращает:
            Строку с C++ типом
        """
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
        """
        Генерирует тестовое значение для заданного C++ типа.

        Аргументы:
            cpp_type: C++ тип поля
            field_name: Имя поля

        Возвращает:
            Строковое представление тестового значения
        """
        if cpp_type in self.dto_names:
            return f"{cpp_type}()"

        if cpp_type.startswith('std::vector<'):
            return "{}"

        if cpp_type == 'bool':
            return 'true'
        if cpp_type in ['int64_t', 'int32_t']:
            return '42'
        if cpp_type in ['double', 'float']:
            return '3.14'
        if cpp_type == 'std::string':
            return '"test_value"'
        if cpp_type == 'std::chrono::system_clock::time_point':
            return 'secondsToTimePoint(1640995200)'

        return '{}'

    def get_json_for_field(self, field: Dict[str, Any]) -> str:
        """
        Генерирует JSON представление для поля в тесте.

        Аргументы:
            field: Словарь с информацией о поле

        Возвращает:
            Строку с JSON фрагментом или комментарием
        """
        field_name = field['name_snake']
        cpp_type = field['cpp_type']

        if field.get('const_value'):
            return f'    // "{field_name}" — константа, пропускаем в тесте'

        if cpp_type == 'std::string':
            return f'    {{"{field_name}", "test_{field_name}"}}'
        elif cpp_type in ['int64_t', 'int32_t']:
            return f'    {{"{field_name}", 42}}'
        elif cpp_type == 'bool':
            return f'    {{"{field_name}", true}}'
        elif cpp_type in ['double', 'float']:
            return f'    {{"{field_name}", 3.14}}'
        elif cpp_type == 'std::chrono::system_clock::time_point':
            return f'    {{"{field_name}", 1640995200}}'
        elif cpp_type.startswith('std::vector<'):
            return f'    {{"{field_name}", nlohmann::json::array({{"item1", "item2"}})}}'
        elif cpp_type in self.dto_names:
            return f'    {{"{field_name}", {cpp_type}().toJson()}}'
        else:
            return f'    {{"{field_name}", "test_value"}}'

    def to_snake_case(self, name: str) -> str:
        """
        Преобразует camelCase в snake_case.

        Аргументы:
            name: Имя в camelCase

        Возвращает:
            Имя в snake_case
        """
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z]+)', r'\1_\2', s1)
        return s2.lower()

    def collect_dtos(self) -> List[Dict[str, Any]]:
        """
        Собирает все DTO из схем OpenAPI.

        Возвращает:
            Список словарей с информацией о DTO
        """
        schemas = self.spec.get('components', {}).get('schemas', {})
        dtos = []

        for name, schema in schemas.items():
            if schema.get('type') == 'object' and 'properties' in schema:
                dto = self.process_dto(name, schema)
                dtos.append(dto)
                self.dto_names.append(name)

        return dtos

    def process_dto(self, name: str, schema: Dict[str, Any]) -> Dict[str, Any]:
        """
        Обрабатывает одну схему DTO для тестирования.

        Аргументы:
            name: Имя DTO
            schema: Схема DTO из OpenAPI

        Возвращает:
            Словарь с информацией о DTO
        """
        properties = schema.get('properties', {})
        required = schema.get('required', [])

        fields = []
        for prop_name, prop_schema in properties.items():
            if prop_schema.get('writeOnly'):
                continue

            cpp_type = self.get_cpp_type(prop_schema, prop_name)

            array_item_type = ""
            if prop_schema.get('type') == 'array':
                items = prop_schema.get('items', {})
                array_item_type = self.get_cpp_type(items)

            const_value = None
            if prop_name == 'tokenType' and prop_schema.get('default') == 'Bearer':
                const_value = 'Bearer'

            field = {
                'name': prop_name,
                'name_snake': self.to_snake_case(prop_name),
                'name_camel': prop_name,
                'name_pascal': prop_name[0].upper() + prop_name[1:] if prop_name else prop_name,
                'cpp_type': cpp_type,
                'array_item_type': array_item_type,
                'required': prop_name in required,
                'description': prop_schema.get('description', ''),
                'const_value': const_value,
            }
            fields.append(field)

        return {
            'name': name,
            'name_snake': self.to_snake_case(name),
            'description': schema.get('description', f'{name} DTO'),
            'fields': fields,
        }

    def collect_includes(self, dto: Dict[str, Any]) -> List[str]:
        """
        Собирает список include-директив для тестового файла.

        Аргументы:
            dto: Словарь с информацией о DTO

        Возвращает:
            Список строк с include-директивами
        """
        includes = [
            '#include <chrono>',
            '#include <ctime>',
            '#include <iomanip>',
            '#include <sstream>',
            '',
            '#include <boost/test/unit_test.hpp>',
            '#include <nlohmann/json.hpp>',
            '',
            f'#include "common/dto/{dto["name_snake"]}.h"',
            '',
        ]

        for field in dto['fields']:
            cpp_type = field['cpp_type']
            if cpp_type in self.dto_names:
                includes.append(f'#include "common/dto/{self.to_snake_case(cpp_type)}.h"')
            elif cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]
                if inner_type in self.dto_names:
                    includes.append(f'#include "common/dto/{self.to_snake_case(inner_type)}.h"')

        return includes

    def generate_test_file(self, dto: Dict[str, Any], template: Template) -> str:
        """
        Генерирует тестовый файл для DTO.

        Аргументы:
            dto: Словарь с информацией о DTO
            template: Шаблон Jinja2

        Возвращает:
            Сгенерированный код тестового файла
        """
        includes = self.collect_includes(dto)

        json_parts = []
        for field in dto['fields']:
            json_part = self.get_json_for_field(field)
            if not json_part.startswith('    //'):
                json_parts.append(json_part)

        json_object = ",\n".join(json_parts)

        field_checks = []
        for field in dto['fields']:
            if field['required'] and not field.get('const_value'):
                if field['cpp_type'] == 'std::string':
                    field_checks.append(f'    BOOST_TEST(!dto.{field["name_camel"]}().empty());')
                elif field['cpp_type'].startswith('std::vector<'):
                    field_checks.append(f'    BOOST_TEST(!dto.{field["name_camel"]}().empty());')
                elif field['cpp_type'] == 'std::chrono::system_clock::time_point':
                    field_checks.append(f'    BOOST_TEST(dto.{field["name_camel"]}().time_since_epoch().count() != 0);')
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
        """
        Записывает содержимое в файл.

        Аргументы:
            filepath: Путь к файлу
            content: Содержимое для записи
        """
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Сгенерирован: {filepath}")

    def run(self) -> None:
        """Запускает процесс генерации тестов DTO."""
        print("Загрузка OpenAPI спецификации...")
        self.load_spec()

        print("Сбор DTO...")
        dtos = self.collect_dtos()
        print(f"Найдено {len(dtos)} DTO")

        env = Environment(
            loader=FileSystemLoader(str(self.templates_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
        )

        env.filters['length'] = lambda x: len(x)

        test_template = env.get_template('dto_test.cpp.j2')

        for dto in dtos:
            test_content = self.generate_test_file(dto, test_template)
            test_file = self.output_dir / f'test_{dto["name_snake"]}.cpp'
            self.write_file(test_file, test_content)

        self.generate_test_runner(dtos)

        print("\nГенерация тестов DTO завершена!")

    def generate_test_runner(self, dtos: List[Dict[str, Any]]) -> None:
        """
        Генерирует главный файл запуска тестов.

        Аргументы:
            dtos: Список DTO
        """
        runner_content = f"""// Автоматически сгенерированный главный файл тестов DTO
// Сгенерирован: {datetime.now().isoformat()}
// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ

#define BOOST_TEST_MODULE FaradoDtoTests
#include <boost/test/unit_test.hpp>

// Тестовые случаи находятся в отдельных файлах
// Этот файл только определяет модуль тестов
"""
        runner_file = self.output_dir / 'test_main.cpp'
        self.write_file(runner_file, runner_content)


def main():
    """Точка входа в скрипт генератора тестов DTO."""
    parser = argparse.ArgumentParser(
        description='Генерирует C++ юнит-тесты для DTO классов с использованием Boost.Test'
    )
    parser.add_argument(
        '--openapi',
        required=True,
        type=Path,
        help='Путь к OpenAPI YAML файлу'
    )
    parser.add_argument(
        '--output',
        required=True,
        type=Path,
        help='Директория для вывода тестов'
    )
    parser.add_argument(
        '--templates',
        required=True,
        type=Path,
        help='Директория с Jinja2 шаблонами'
    )

    args = parser.parse_args()

    if not args.openapi.exists():
        print(f"Ошибка: OpenAPI файл не найден: {args.openapi}")
        return 1

    if not args.templates.exists():
        print(f"Ошибка: Директория с шаблонами не найдена: {args.templates}")
        return 1

    generator = DTOTestGenerator(args.openapi, args.output, args.templates)
    generator.run()

    return 0


if __name__ == '__main__':
    exit(main())
