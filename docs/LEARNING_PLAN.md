# План обучения: что делает JARVIS и что учишь ты

Этот документ объясняет **что уже сделано в коде**, **что будешь делать ты** и **что выучишь** по ходу.

---

## Архитектура (большая картина)

```text
Твои .md файлы
      ↓
  jarvis index
      ↓
 SQLite (jarvis.db)
  ├─ documents / chunks / embeddings  ← RAG
  ├─ chat_history                   ← память диалогов
  └─ user_facts                     ← профиль (Максим, СибГУ...)
      ↓
  Запрос в чате
      ↓
  Поиск похожих чанков (cosine similarity)
      ↓
  Ollama (llama3.2) + tools
      ↓
  Ответ JARVIS
```

---

## Блок 1 — RAG (база знаний)

### Что делает код
- Читает `data/documents/*.md`
- Режет на чанки (UTF-8 безопасно)
- Ollama создаёт embeddings
- Сохраняет в SQLite
- При вопросе ищет похожие чанки

### Что делаешь ты
- Заполняешь шаблоны: `about_me.md`, `study.md`, `projects.md`...
- Запускаешь `jarvis index`
- Тестируешь вопросы, пишешь в `test_log.md`

### Что учишь
- Что такое RAG и зачем он нужен
- Чанки, эмбеддинги, cosine similarity
- Как «обучение» без переобучения модели = наполнение документов

---

## Блок 2 — SQLite

### Что делает код
- `database.cpp` — таблицы, CRUD, BLOB для векторов
- Миграция из старого `chunks.json`
- `chat_history`, `user_facts`

### Что делаешь ты
- Устанавливаешь DB Browser for SQLite
- Открываешь `data/jarvis.db`, смотришь таблицы
- Пишешь 5 SQL-запросов в тетрадь (см. WEEKLY_PLAN.md)

### Что учишь
- CREATE TABLE, INSERT, SELECT, JOIN
- Связь 1-ко-многим (document → chunks)
- BLOB для бинарных данных (векторы)
- Зачем БД лучше JSON при росте проекта

---

## Блок 3 — Tools (инструменты)

### Что делает код
- `get_time`, `read_file`, `list_directory`, `run_command`
- Tool-calling loop в `agent.cpp`
- Для личных вопросов tools отключены

### Что делаешь ты
- Тестируешь: «какое время», «прочитай файл X»
- Позже — добавишь свой tool (заметки, дедлайны)

### Что учишь
- Function calling у LLM
- Когда tools нужны, а когда достаточно RAG
- Безопасность (whitelist команд)

---

## Блок 4 — Ollama и агент

### Что делает код
- HTTP к локальной LLM
- Системный промпт + контекст из RAG
- История в памяти + SQLite

### Что делаешь ты
- `ollama pull`, `jarvis status`
- Ведёшь `dialog_examples.md` (20+ примеров)

### Что учишь
- Локальные LLM без облака
- Роль system / user / assistant
- Промпт-инжиниринг на практике

---

## Блок 5 — Fine-tune (будущее)

### Что сделаешь позже
- Собрать `data/finetune/dataset.jsonl` из диалогов
- LoRA на llama3.2

### Что учишь
- Разница RAG vs fine-tune
- Формат JSONL для обучения
- Когда имеет смысл переобучать

---

## Твои еженедельные действия

| Действие | Частота |
|----------|---------|
| Обновить 1+ документ в `data/documents/` | 1–2 раза в неделю |
| `jarvis index` | после каждого изменения документов |
| 5 тестовых вопросов → `test_log.md` | 1 раз в неделю |
| 1–2 SQL-запроса к `jarvis.db` | при изучении БД |
| +2 примера в `dialog_examples.md` | 1 раз в неделю |

---

## Команды — шпаргалка

```powershell
Set-Location "D:\projects\my-ai-agent"
powershell -ExecutionPolicy Bypass -File .\build.ps1
.\build\Release\jarvis.exe index
.\build\Release\jarvis.exe status
.\build\Release\jarvis.exe facts
.\build\Release\jarvis.exe history
.\build\Release\jarvis.exe ask "кто я бро?"
.\build\Release\jarvis.exe
```

---

## Критерий «я понял»

После 4–6 недель ты можешь объяснить:

1. Как документ превращается в чанк и embedding  
2. Как JARVIS находит ответ в SQLite  
3. Зачем `user_facts` и `chat_history`  
4. Почему для «кто я» нужен RAG, а не tool  
5. Чем fine-tune отличается от RAG  
