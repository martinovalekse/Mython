# Интерпретатор языка Mython

## учебный проект

Интрепретатор языка Mython, упрощённого подмножества Python. В нем есть классы и наследование, а все методы — виртуальные. Cоздавался с целью получить навыки необходимые для создания собственного DSL и закрепления полученных на курсе по С++ знаний в целом.

Программа принимает первым аргументом принимает файл с корректным кодом на языке Mython, а в файл во втором аргументе выводит результат выполнения этого кода.

Написано на с++ '17.

## реализация
Интерпретатор состоит из множества отдельных модулей:

- **runtime** - модуль интерпретатора, отвечающий за управление состоянием программы во время её работы. Этот модуль реализует встроенные типы данных языка Mython и таблицу символов.
- **lexer** — лексический анализатор для разбора программы на языке Mython. Преобразует корректный код в последовательность токенов.
- **parse** — синтаксический анализатор (парсер) языка Mython (В учебном задании этот модуль предоставлен авторами. Его реализация требует определённой теоретической подготовки, выходящей за рамки пройденого курса).
- **statement** - объявления классов узлов абстрактного синтаксического дерева (AST). Парсер использует эти классы в процессе построения AST. Объединяет три основных модуля.

statement_test.cpp, parse_test.cpp, runtime_test.cpp, lexer_test_open.cpp - файлы юнит-тестов для компонентов интерпретатора.
В файле test_runner.h — классы и макросы, необходимые для работы тестов.

_К проекту приложен Mython_help.pdf кратко описывающий синтаксис языка. test.my - пример корректного кода._





