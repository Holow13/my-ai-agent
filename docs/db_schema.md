# Схема базы данных JARVIS

Файл: `data/jarvis.db` (SQLite)

## Таблицы

### documents
Исходные файлы базы знаний.

| Поле | Тип | Описание |
|------|-----|----------|
| id | INTEGER PK | ID документа |
| path | TEXT UNIQUE | Путь к файлу |
| title | TEXT | Имя файла |
| indexed_at | TEXT | Время индексации |

### chunks
Куски текста для RAG.

| Поле | Тип | Описание |
|------|-----|----------|
| id | INTEGER PK | ID чанка |
| document_id | INTEGER FK | Ссылка на documents |
| position | INTEGER | Порядок в документе |
| text | TEXT | Текст чанка |

### embeddings
Векторы эмбеддингов (BLOB из float[]).

| Поле | Тип | Описание |
|------|-----|----------|
| chunk_id | INTEGER PK FK | Ссылка на chunks |
| vector | BLOB | Массив float |

### chat_history
История диалогов.

| Поле | Тип | Описание |
|------|-----|----------|
| id | INTEGER PK | ID сообщения |
| role | TEXT | user / assistant |
| content | TEXT | Текст |
| created_at | TEXT | Время |

### user_facts
Профиль пользователя (ключ–значение).

| Поле | Тип | Описание |
|------|-----|----------|
| key | TEXT PK | name, city, university... |
| value | TEXT | Значение |

## Связи

```text
documents 1──* chunks 1──1 embeddings
```

## Команды

```powershell
.\build\Release\jarvis.exe index    # заполнить documents/chunks/embeddings
.\build\Release\jarvis.exe history  # chat_history
.\build\Release\jarvis.exe facts     # user_facts
```

## Просмотр в DB Browser

1. Установить DB Browser for SQLite
2. Открыть `D:\projects\my-ai-agent\data\jarvis.db`
3. Вкладка «Обзор данных» → таблицы

## Миграция со старого JSON

Если есть `data/index/chunks.json`, при первом `load_index` данные импортируются в SQLite автоматически.
