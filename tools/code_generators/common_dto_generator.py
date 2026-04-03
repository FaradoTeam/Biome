#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Генератор DTO-классов на C++ из OpenAPI спецификации.
Создаёт классы с поддержкой сериализации/десериализации в JSON.
"""

import argparse
import yaml
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
from jinja2 import Environment, FileSystemLoader, Template
import re


class DTOGenerator:
    """
    Генерирует C++ DTO классы из OpenAPI спецификации.

    Атрибуты:
        openapi_file: Путь к файлу OpenAPI спецификации
        output_dir: Директория для сохранения сгенерированных файлов
        templates_dir: Директория с шаблонами Jinja2
        spec: Загруженная OpenAPI спецификация
        dto_names: Список имён сгенерированных DTO
    """

    # Соответствие OpenAPI типов C++ типам
    TYPE_MAPPINGS = {
        'integer': 'int64_t',
        'string': 'std::string',
        'boolean': 'bool',
        'number': 'double',
        'array': 'std::vector',
        'object': 'nlohmann::json',
    }

    # Соответствие форматов OpenAPI C++ типам
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
        """
        Инициализирует генератор DTO.

        Аргументы:
            openapi_file: Путь к OpenAPI YAML файлу
            output_dir: Директория для вывода сгенерированных файлов
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
        Преобразует OpenAPI тип в соответствующий C++ тип.

        Аргументы:
            schema: Схема OpenAPI поля
            name: Имя поля (используется для отладки)

        Возвращает:
            Строку с C++ типом
        """
        if '$ref' in schema:
            # Извлекаем имя типа из ссылки
            ref = schema['$ref']
            type_name = ref.split('/')[-1]
            return type_name

        schema_type = schema.get('type', 'string')
        schema_format = schema.get('format', '')

        # Проверяем формат в первую очередь
        if schema_format in self.FORMAT_MAPPINGS:
            return self.FORMAT_MAPPINGS[schema_format]

        # Обрабатываем массивы
        if schema_type == 'array':
            items = schema.get('items', {})
            item_type = self.get_cpp_type(items)
            return f"std::vector<{item_type}>"

        # Обрабатываем объекты
        if schema_type == 'object':
            if 'properties' in schema:
                # Вложенный объект — обрабатываем как JSON
                return 'nlohmann::json'
            return 'nlohmann::json'

        # Обрабатываем перечисления
        if 'enum' in schema:
            return 'std::string'

        # Базовые типы
        return self.TYPE_MAPPINGS.get(schema_type, 'std::string')

    def get_array_item_type(self, schema: Dict[str, Any]) -> str:
        """
        Возвращает тип элемента для массива.

        Аргументы:
            schema: Схема массива

        Возвращает:
            C++ тип элемента массива
        """
        items = schema.get('items', {})
        return self.get_cpp_type(items)

    def is_required(self, schema_name: str, property_name: str) -> bool:
        """
        Проверяет, является ли свойство обязательным.

        Аргументы:
            schema_name: Имя схемы
            property_name: Имя свойства

        Возвращает:
            True если свойство обязательно, иначе False
        """
        schemas = self.spec.get('components', {}).get('schemas', {})
        schema = schemas.get(schema_name, {})
        required = schema.get('required', [])
        return property_name in required

    def get_default_value(self, schema: Dict[str, Any]) -> Optional[str]:
        """
        Возвращает значение по умолчанию для свойства.

        Аргументы:
            schema: Схема поля

        Возвращает:
            Строковое представление значения по умолчанию или None
        """
        if 'default' in schema:
            default = schema['default']
            if isinstance(default, str):
                return f'"{default}"'
            elif isinstance(default, bool):
                return 'true' if default else 'false'
            return str(default)
        return None

    def get_enum_values(self, schema: Dict[str, Any]) -> List[str]:
        """
        Возвращает список значений перечисления.

        Аргументы:
            schema: Схема поля

        Возвращает:
            Список строковых значений enum
        """
        if 'enum' in schema:
            return [str(v) for v in schema['enum']]
        return []

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

    def to_camel_case(self, name: str) -> str:
        """
        Преобразует snake_case в lowerCamelCase.
        Сохраняет оригинальное имя из OpenAPI.

        Аргументы:
            name: Имя в любом формате

        Возвращает:
            Имя в lowerCamelCase
        """
        return name

    def to_pascal_case(self, name: str) -> str:
        """
        Преобразует имя в PascalCase для методов-сеттеров.

        Аргументы:
            name: Исходное имя

        Возвращает:
            Имя с заглавной первой буквой
        """
        if name and not name[0].isupper():
            return name[0].upper() + name[1:]
        return name

    def collect_dtos(self) -> List[Dict[str, Any]]:
        """
        Собирает все DTO из схем OpenAPI.

        Возвращает:
            Список словарей с информацией о каждом DTO
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
        Обрабатывает одну схему DTO.

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
            # Пропускаем поля только для записи (например, пароль)
            if prop_schema.get('writeOnly'):
                continue

            is_required = prop_name in required
            cpp_type = self.get_cpp_type(prop_schema, prop_name)

            array_item_type = ""
            if prop_schema.get('type') == 'array':
                array_item_type = self.get_array_item_type(prop_schema)

            default = self.get_default_value(prop_schema)
            enum_values = self.get_enum_values(prop_schema)

            const_value = None
            if prop_name == 'tokenType' and prop_schema.get('default') == 'Bearer':
                const_value = 'Bearer'

            name_snake = self.to_snake_case(prop_name)
            name_camel = prop_name
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
        """
        Собирает зависимости DTO от других DTO.

        Аргументы:
            dto: Словарь с информацией о DTO

        Возвращает:
            Список имён зависимых DTO
        """
        dependencies = set()

        for field in dto['fields']:
            cpp_type = field['cpp_type']

            if cpp_type in self.dto_names:
                dependencies.add(cpp_type)

            if cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]
                if inner_type in self.dto_names:
                    dependencies.add(inner_type)

        return list(dependencies)

    def collect_includes(self, dto: Dict[str, Any]) -> List[str]:
        """
        Собирает список include-директив для DTO.

        Аргументы:
            dto: Словарь с информацией о DTO

        Возвращает:
            Список строк с include-директивами
        """
        includes = [
            '#include <chrono>',
            '#include <ctime>',
            '#include <memory>',
            '#include <string>',
            '#include <vector>',
            '#include <unordered_map>',
            '',
            '#include <nlohmann/json.hpp>',
            '',
        ]

        for field in dto['fields']:
            cpp_type = field['cpp_type']

            if cpp_type in self.dto_names:
                includes.append(f'#include "{self.to_snake_case(cpp_type)}.h"')

            if cpp_type.startswith('std::vector<'):
                inner_type = cpp_type[12:-1]
                if inner_type in self.dto_names:
                    includes.append(f'#include "{self.to_snake_case(inner_type)}.h"')

        return includes

    def generate_dto_header(self, dto: Dict[str, Any], template: Template) -> str:
        """
        Генерирует заголовочный файл DTO.

        Аргументы:
            dto: Словарь с информацией о DTO
            template: Шаблон Jinja2 для заголовка

        Возвращает:
            Сгенерированный код заголовочного файла
        """
        includes = self.collect_includes(dto)
        dependencies = self.collect_dependencies(dto)

        return template.render(
            dto=dto,
            includes=includes,
            dependencies=dependencies,
            generated_at=datetime.now().isoformat(),
        )

    def generate_dto_impl(self, dto: Dict[str, Any], template: Template) -> str:
        """
        Генерирует файл реализации DTO.

        Аргументы:
            dto: Словарь с информацией о DTO
            template: Шаблон Jinja2 для реализации

        Возвращает:
            Сгенерированный код файла реализации
        """
        return template.render(
            dto=dto,
            generated_at=datetime.now().isoformat(),
        )

    def generate_common_header(self, template: Template) -> str:
        """
        Генерирует общий заголовочный файл с forward-объявлениями.

        Аргументы:
            template: Шаблон Jinja2

        Возвращает:
            Сгенерированный код общего заголовка
        """
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
        """Запускает процесс генерации DTO."""
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

        header_template = env.get_template('dto.h.j2')
        impl_template = env.get_template('dto.cpp.j2')
        common_template = env.get_template('dto_common.h.j2')

        common_header_content = self.generate_common_header(common_template)
        self.write_file(self.output_dir / 'dto_common.h', common_header_content)

        for dto in dtos:
            header_content = self.generate_dto_header(dto, header_template)
            header_file = self.output_dir / f'{self.to_snake_case(dto["name"])}.h'
            self.write_file(header_file, header_content)

            impl_content = self.generate_dto_impl(dto, impl_template)
            impl_file = self.output_dir / f'{self.to_snake_case(dto["name"])}.cpp'
            self.write_file(impl_file, impl_content)

        print("\nГенерация DTO завершена!")


def main():
    """Точка входа в скрипт генератора DTO."""
    parser = argparse.ArgumentParser(
        description='Генерирует C++ DTO классы из OpenAPI спецификации'
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
        help='Директория для вывода сгенерированных файлов'
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

    generator = DTOGenerator(args.openapi, args.output, args.templates)
    generator.run()

    return 0


if __name__ == '__main__':
    exit(main())
