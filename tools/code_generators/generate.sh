#/bin/bash

python3 common_dto_generator.py \
    --openapi ../../api/openapi.yaml \
    --output ../../common/dto \
    --templates ./templates

python3 test_dto_generator.py \
    --openapi ../../api/openapi.yaml \
    --output ../../tests/dto \
    --templates ./templates
