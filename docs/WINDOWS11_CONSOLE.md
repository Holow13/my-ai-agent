# ТЗ: запуск JARVIS через консоль (Windows 11)

Пошаговая инструкция для **PowerShell** / **Windows Terminal**.

---

## 1. Куда ставить проект

| Что | Рекомендуемый путь | Зачем |
|-----|-------------------|-------|
| Проект | `D:\projects\my-ai-agent` | SSD, отдельно от системных папок |
| Если диска D: нет | `C:\dev\my-ai-agent` | Короткий путь, быстрый доступ |
| Ваши заметки (RAG) | `D:\projects\my-ai-agent\data\documents` | По умолчанию, менять не нужно |
| Модели Ollama | `C:\Users\<ИМЯ>\.ollama` | Ставится автоматически |

**Минимум свободного места на SSD:** 20 GB (проект + 2 модели + запас).

---

## 2. Установка зависимостей (один раз)

Откройте **PowerShell от имени пользователя** (не обязательно администратор) и выполните:

```powershell
# Git
winget install --id Git.Git -e

# CMake
winget install --id Kitware.CMake -e

# Ollama
winget install --id Ollama.Ollama -e

# Компилятор C++ (Visual Studio Build Tools) — PowerShell от администратора
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Если команда выше не поставила C++, откройте вручную:

1. Пуск → **Visual Studio Installer**
2. У **Build Tools 2022** нажмите **Изменить** (Modify)
3. Поставьте галочку **Разработка классических приложений на C++**
4. Справа убедитесь, что выбраны **MSVC** и **Windows SDK**
5. Нажмите **Установить**

После установки **перезагрузите ПК**, затем откройте новый PowerShell.

Проверка:

```powershell
git --version
cmake --version
ollama --version
```

---

## 3. Создать папку и скачать проект

```powershell
# Создать папку (выберите один вариант)

# Вариант A — диск D:
New-Item -ItemType Directory -Force -Path "D:\projects"
Set-Location "D:\projects"

# Вариант B — диск C:
# New-Item -ItemType Directory -Force -Path "C:\dev"
# Set-Location "C:\dev"

# Клонировать репозиторий
git clone https://github.com/Holow13/my-ai-agent.git
Set-Location "my-ai-agent"
```

---

## 4. Скачать модели Ollama

```powershell
# Запустить Ollama (если не запущена — иконка в трее)
ollama serve
```

В **новом окне PowerShell**:

```powershell
ollama pull llama3.2
ollama pull nomic-embed-text
```

Проверка:

```powershell
ollama list
```

---

## 5. Сборка проекта

Если PowerShell пишет `выполнение сценариев отключено`, сначала выполните:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

Или запускайте скрипт без смены политики:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

```powershell
# Перейти в папку проекта (подставьте свой путь)
Set-Location "D:\projects\my-ai-agent"

# Вариант A — скрипт (рекомендуется)
.\build.ps1

# Вариант B — вручную (явно указываем генератор VS, не nmake)
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
```

> **Важно:** не запускайте просто `cmake -S . -B build` без `-G` — на Windows это часто ломается с ошибкой `nmake`.

Готовый файл:

```text
D:\projects\my-ai-agent\build\Release\jarvis.exe
```

---

## 6. Первый запуск

```powershell
Set-Location "D:\projects\my-ai-agent"

# Проверить статус
.\build\Release\jarvis.exe status

# Проиндексировать ваши документы
.\build\Release\jarvis.exe index

# Интерактивный чат
.\build\Release\jarvis.exe
```

Один вопрос без входа в чат:

```powershell
.\build\Release\jarvis.exe ask "кто я?"
```

Выход из чата: введите `exit` или `quit`.

---

## 7. Добавить свои документы

1. Положите файлы `.md` и `.txt` в папку:

```text
D:\projects\my-ai-agent\data\documents\
```

2. Переиндексируйте:

```powershell
Set-Location "D:\projects\my-ai-agent"
.\build\Release\jarvis.exe index
```

---

## 8. Ежедневный запуск (шаблон)

Каждый раз при работе:

```powershell
# 1. Убедиться, что Ollama запущена (трей или команда)
ollama serve

# 2. Перейти в проект
Set-Location "D:\projects\my-ai-agent"

# 3. Запустить ассистента
.\build\Release\jarvis.exe
```

---

## 9. Полезные команды

| Команда | Что делает |
|---------|------------|
| `.\build\Release\jarvis.exe status` | Ollama онлайн? Индекс есть? |
| `.\build\Release\jarvis.exe index` | Пересобрать базу знаний |
| `.\build\Release\jarvis.exe ask "текст"` | Один запрос |
| `.\build\Release\jarvis.exe` | Диалоговый режим |

---

## 10. Если что-то не работает

### `ollama` не распознано (CommandNotFoundException)

PowerShell не видит Ollama. Сделайте по шагам:

**Шаг 1 — проверить, установлена ли Ollama:**

```powershell
Get-Command ollama -ErrorAction SilentlyContinue
Test-Path "$env:LOCALAPPDATA\Programs\Ollama\ollama.exe"
```

**Шаг 2 — если `False`, установить:**

```powershell
winget install --id Ollama.Ollama -e
```

Или скачайте установщик: https://ollama.com/download/windows

**Шаг 3 — закрыть PowerShell полностью и открыть новое окно.**

**Шаг 4 — если всё ещё не работает, запускать по полному пути:**

```powershell
& "$env:LOCALAPPDATA\Programs\Ollama\ollama.exe" --version
& "$env:LOCALAPPDATA\Programs\Ollama\ollama.exe" pull llama3.2
& "$env:LOCALAPPDATA\Programs\Ollama\ollama.exe" pull nomic-embed-text
```

**Шаг 5 — добавить Ollama в PATH (один раз):**

```powershell
$ollamaDir = "$env:LOCALAPPDATA\Programs\Ollama"
[Environment]::SetEnvironmentVariable(
    "Path",
    [Environment]::GetEnvironmentVariable("Path", "User") + ";$ollamaDir",
    "User"
)
```

Закройте и снова откройте PowerShell, затем:

```powershell
ollama --version
```

**Шаг 6 — убедиться, что Ollama запущена:**

- В трее (возле часов) должна быть иконка llama, или
- Запустите из меню Пуск → **Ollama**

---

### `Ollama: offline`

```powershell
# Запустить сервер
ollama serve

# Или перезапустить из трея (правый клик по иконке Ollama → Quit → снова открыть Ollama)
```

### `cannot open config.json`

Запускайте `jarvis.exe` из корня проекта, где лежит `config.json`:

```powershell
Set-Location "D:\projects\my-ai-agent"
.\build\Release\jarvis.exe status
```

### `could not find any instance of Visual Studio`

Компилятор C++ не установлен. В PowerShell **от администратора**:

```powershell
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Потом перезагрузите ПК и снова:

```powershell
Set-Location "D:\projects\my-ai-agent"
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

---

### `nmake` / `no such file or directory` при cmake

CMake выбрал не тот генератор. Исправление:

```powershell
Set-Location "D:\projects\my-ai-agent"
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
```

Если ошибка остаётся — установите Build Tools:

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools -e
```

В **Visual Studio Installer** включите: **Desktop development with C++**, перезагрузите ПК, затем:

```powershell
.\build.ps1
```

### `cmake` не найден

Перезапустите PowerShell. Если не помогло — добавьте CMake в PATH вручную:

```text
C:\Program Files\CMake\bin
```

### Документы на другом диске

Отредактируйте `config.json`:

```json
{
  "documents_dir": "D:/ai/knowledge"
}
```

Потом снова:

```powershell
.\build\Release\jarvis.exe index
```

---

## 11. Быстрый скрипт запуска (опционально)

Создайте файл `D:\projects\my-ai-agent\start.ps1`:

```powershell
Set-Location $PSScriptRoot
Start-Process "ollama" -ArgumentList "serve" -WindowStyle Hidden
Start-Sleep -Seconds 2
.\build\Release\jarvis.exe
```

Запуск:

```powershell
Set-Location "D:\projects\my-ai-agent"
.\start.ps1
```

Если PowerShell блокирует скрипт:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

---

## Итог

```powershell
# Установка (один раз)
winget install Git.Git Kitware.CMake Ollama.Ollama Microsoft.VisualStudio.2022.BuildTools

# Проект
Set-Location "D:\projects"
git clone https://github.com/Holow13/my-ai-agent.git
Set-Location "my-ai-agent"

# Модели
ollama pull llama3.2
ollama pull nomic-embed-text

# Сборка и запуск
.\build.ps1
.\build\Release\jarvis.exe index
.\build\Release\jarvis.exe
```
