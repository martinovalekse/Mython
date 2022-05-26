# Интерпретатор языка Mython

## учебный проект.

Mython упрощённое подмножество языка Python. В нем есть классы и наследование, а все методы — виртуальные. Его интерпретатор создавался с целью получить навыки которые пригодятся, когда нужно будет создать собственный DSL и закрепления полученных на курсе по С++ знаний в целом.

Программа принимает во входящий поток с корректным кодом на языке Mython, а в выходящий поток выводит результат выполнения этого кода.

Написано на с++ '17.

## реализация
Интерпретатор состоит из множества отдельных модулей:

- **runtime** - модуль интерпретатора, отвечающий за управление состоянием программы во время её работы. Этот модуль реализует встроенные типы данных языка Mython и таблицу символов.
- **lexer** — лексический анализатор для разбора программы на языке Mython. Преобразует корректный код в последовательность токенов.
- **parse** — синтаксический анализатор (парсер) языка Mython (В учебном задании этот модуль предоставлен авторами. Для его написания требуется определённая теоретическая подготовка, которая выходила за рамки этого курса по C++).
- **statement** - объявления классов узлов абстрактного синтаксического дерева (AST). Парсер использует эти классы в процессе построения AST. Объединяет три основных модуля.

statement_test.cpp, parse_test.cpp, runtime_test.cpp, lexer_test_open.cpp - файлы юнит-тестов для компонентов интерпретатора.
В файле test_runner.h — классы и макросы, необходимые для работы тестов.

_К проекту приложен Mython_help.pdf кратко описывающий синтаксис языка._





