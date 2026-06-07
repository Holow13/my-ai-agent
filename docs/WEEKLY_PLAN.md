# Понедельный план (Максим, СибГУ, ИиВТ)

Подробный план: что делать, что должно получиться, что учить.

---

## Неделя 1 — Документы и тесты

### Твои задачи
1. Заполнить шаблоны в `data/documents/` (уже созданы — **отредактируй под себя**)
2. Каждый день после правок: `jarvis index`
3. Заполнить `test_log.md` (минимум 10 вопросов)

### Команды
```powershell
git pull origin main
powershell -ExecutionPolicy Bypass -File .\build.ps1
.\build\Release\jarvis.exe index
.\build\Release\jarvis.exe ask "кто я бро?"
```

### Результат недели
- [ ] 7+ файлов в documents (шаблоны + свои заметки)
- [ ] JARVIS знает: Максим, СибГУ, ИиВТ, Новосибирск
- [ ] `test_log.md` заполнен

### Что учишь
- RAG на практике
- UTF-8 и кодировки на Windows

---

## Неделя 2 — SQL и схема БД

### Твои задачи
1. Установить [DB Browser for SQLite](https://sqlitebrowser.org/)
2. Прочитать `docs/db_schema.md`
3. Открыть `data/jarvis.db` после `index`
4. Выполнить в DB Browser:

```sql
SELECT * FROM user_facts;
SELECT d.title, COUNT(c.id) FROM documents d JOIN chunks c ON c.document_id = d.id GROUP BY d.id;
SELECT role, content, created_at FROM chat_history ORDER BY id DESC LIMIT 5;
```

### Результат недели
- [ ] 5 SQL-запросов выполнены и записаны в тетрадь
- [ ] Понимаешь таблицы documents / chunks / embeddings
- [ ] +3 своих документа в `data/documents/`

### Что учишь
- SELECT, JOIN, COUNT
- Проектирование схемы БД

---

## Неделя 3 — SQLite в проекте (уже в коде)

### Что сделано в репозитории
- `jarvis.db` вместо `chunks.json`
- Миграция из старого JSON
- `jarvis history`, `jarvis facts`

### Твои задачи
1. `git pull` + пересборка
2. `jarvis index` — проверить, что данные в `.db`
3. Удалить `data/index/chunks.json` (опционально) — снова `index`, всё работает

### Результат недели
- [ ] Индекс только в SQLite
- [ ] `jarvis history` показывает диалоги
- [ ] `jarvis facts` показывает профиль

### Что учишь
- Как C++ работает с SQLite
- BLOB для векторов

---

## Неделя 4 — Качество и примеры

### Твои задачи
1. Дописать `dialog_examples.md` до 20 пар
2. Настроить `config.json`: `"rag_top_k": 6`
3. Добавить в `study.md` реальные дедлайны

### Результат недели
- [ ] 8/10 вопросов из test_log — ответ OK
- [ ] 20 примеров в dialog_examples.md

### Что учишь
- Как примеры улучшают ответы без fine-tune
- Настройка RAG-параметров

---

## Неделя 5 — Датасет для fine-tune

### Твои задачи
1. Создать папку `data/finetune/`
2. Копировать удачные диалоги в `dataset.jsonl`:

```json
{"messages":[{"role":"user","content":"кто я бро?"},{"role":"assistant","content":"Максим, 2 курс ИиВТ..."}]}
```

3. Цель: 50+ строк

### Результат недели
- [ ] `dataset.jsonl` начат
- [ ] Понимаешь разницу RAG / fine-tune

---

## Неделя 6 — Финал и портфолио

### Твои задачи
1. Использовать `start.ps1` для запуска
2. Прочитать `docs/MY_JARVIS.md`
3. Сделать 3 скриншота диалогов для отчёта/курса

### Результат недели
- [ ] Запуск в 1 команду
- [ ] Можешь показать проект одногруппнику
- [ ] План на fine-tune или голос

---

## Минимум при нехватке времени

Раз в неделю (~2 часа):
1. Один новый `.md` в documents  
2. `jarvis index`  
3. 5 вопросов в test_log  
4. Один SQL-запрос к jarvis.db  
