# Мой JARVIS — шпаргалка (Максим)

## Пути

| Что | Где |
|-----|-----|
| Проект | `D:\projects\my-ai-agent` |
| Документы | `data\documents\` |
| База данных | `data\jarvis.db` |
| Конфиг | `config.json` |

## Запуск

```powershell
Set-Location "D:\projects\my-ai-agent"
.\start.ps1
```

Или вручную:

```powershell
ollama serve
.\build\Release\jarvis.exe
```

## После изменения документов

```powershell
.\build\Release\jarvis.exe index
```

## Полезные команды

| Команда | Зачем |
|---------|-------|
| `jarvis status` | Ollama + чанки в БД |
| `jarvis facts` | Профиль из user_facts |
| `jarvis history` | Последние сообщения |
| `jarvis ask "..."` | Один вопрос |

## Файлы которые редактирую

- `about_me.md` — кто я
- `study.md` — учёба, дедлайны
- `projects.md` — проекты
- `dialog_examples.md` — примеры для стиля
- `test_log.md` — журнал тестов

## Если сломалось

1. Ollama не работает → запустить Ollama из трея  
2. Пустые ответы → `jarvis index`  
3. Ошибка UTF-8 → сохранить .md как UTF-8 в Блокноте  
4. Сборка → `powershell -ExecutionPolicy Bypass -File .\build.ps1`  

## Документация

- [LEARNING_PLAN.md](LEARNING_PLAN.md) — что учить
- [WEEKLY_PLAN.md](WEEKLY_PLAN.md) — план по неделям
- [db_schema.md](db_schema.md) — схема SQLite
