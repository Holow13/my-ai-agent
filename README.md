# Мой ИИ агент (JARVIS)

Персональный ассистент на **C++** с тремя слоями персонализации:

- **RAG** — ответы из ваших документов (`data/documents/`)
- **Tools** — время, файлы, директории, безопасные shell-команды
- **Ollama** — локальная LLM без облака

## Требования

## Windows 11

Пошаговое ТЗ для консоли (PowerShell): [docs/WINDOWS11_CONSOLE.md](docs/WINDOWS11_CONSOLE.md)

- CMake 3.16+
- C++17 компилятор (GCC, Clang)
- [Ollama](https://ollama.com/)

## Быстрый старт

```bash
# Модели
ollama pull llama3.2
ollama pull nomic-embed-text

# Сборка (g++ рекомендуется)
CXX=g++ cmake -S . -B build
cmake --build build

# Индексация документов
./build/jarvis index

# Чат
./build/jarvis
```

## Команды

| Команда | Описание |
|---------|----------|
| `./build/jarvis` | Интерактивный чат |
| `./build/jarvis ask "вопрос"` | Один запрос |
| `./build/jarvis index` | Пересобрать RAG-индекс |
| `./build/jarvis status` | Статус Ollama и индекса |

## Конфигурация

Файл `config.json`:

- `chat_model` — модель для диалога
- `embed_model` — модель для эмбеддингов
- `documents_dir` — папка с `.md` / `.txt`
- `rag_top_k` — сколько чанков подставлять в контекст

## Архитектура

```
Запрос → RAG (поиск по документам) → Ollama + Tools → Ответ
```

## Следующие шаги

1. Добавить свои документы в `data/documents/`
2. Fine-tune модели под стиль и команды (LoRA)
3. Wake word + STT/TTS для голосового режима
