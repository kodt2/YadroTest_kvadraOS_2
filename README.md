# KVADRA OS Monitor

Веб-приложение для динамического отображения загрузки ресурсов Linux PC в стиле `top`/`htop`.
Проект реализован в рамках тестового задания: frontend написан на WebUI-стеке
**HTML + CSS + TypeScript**, backend — на **C++**. Backend собирает данные из Linux
`/proc` и `/sys`, отдает статические файлы и транслирует метрики в браузер через
WebSocket.

## Что показывает приложение

- текущую загрузку CPU в процентах;
- график CPU с обновлением в реальном времени;
- загрузку RAM
- температуры из `/sys/class/thermal`, если они доступны на хосте;
- топ процессов по потреблению RAM.

## Архитектура

```text
Browser WebUI
  ├─ HTML/CSS интерфейс
  ├─ TypeScript логика
  └─ Chart.js для графика CPU
        │
        │ WebSocket ws://localhost:8081
        │ HTTP http://localhost:8080
        ▼
C++ backend
  ├─ cpp-httplib: HTTP-сервер и отдача ./public
  ├─ IXWebSocket: push-обновления метрик
  ├─ nlohmann/json: JSON-сериализация
  └─ Linux /proc и /sys: источник метрик
```

Основные директории:

- `backend/` — C++ backend, сборка через CMake;
- `frontend/` — HTML/CSS/TypeScript frontend;
- `public/` — генерируемая директория со статикой после сборки;
- `run.sh` — автоматическая установка зависимостей, сборка и запуск;
- `Dockerfile` — контейнерная сборка и запуск приложения;
- `docker-compose.yml` — запуск через Docker Compose.

## Требования

Для запуска без Docker требуется Linux-дистрибутив с `apt`, а также доступ к
установке системных пакетов. Скрипт `run.sh` самостоятельно устанавливает:

- Node.js 20;
- CMake;
- g++;
- zlib;
- OpenSSL;
- nlohmann-json3-dev.

Для контейнерного запуска требуется Docker или Docker Compose.

> Примечание: зависимости C++ библиотек `IXWebSocket`, `cpp-httplib` и
> `nlohmann/json` подтягиваются CMake через `FetchContent`, поэтому на этапе
> сборки нужен доступ в интернет.

## Варианты запуска

Приложение поддерживает три альтернативных сценария запуска: через bash-скрипт,
через Docker и через Docker Compose.

### 1. Запуск через bash-скрипт

Самый простой вариант для локальной Linux-среды:

```bash
chmod +x run.sh
./run.sh
```

Скрипт выполнит следующие шаги:

1. установит системные зависимости;
2. соберет C++ backend в `backend/build/`;
3. установит npm-зависимости frontend;
4. скомпилирует TypeScript;
5. подготовит директорию `public/`;
6. запустит бинарный файл `monitor_backend`.

После запуска откройте интерфейс в браузере:

```text
http://localhost:8080
```

### 2. Запуск через Docker

Соберите образ:

```bash
docker build -t kvadra-os-monitor .
```

Запустите контейнер:

```bash
docker run -d --rm \
  --name kvadra-os-monitor \
  -p 8080:8080 \
  -p 8081:8081 \
  kvadra-os-monitor
```

Откройте приложение:

```text
http://localhost:8080
```

Если нужно приблизить набор метрик к хостовой системе, можно запускать контейнер
с доступом к пространству PID хоста:

```bash
docker run --rm \
  --name kvadra-os-monitor \
  --pid=host \
  -p 8080:8080 \
  -p 8081:8081 \
  kvadra-os-monitor
```

Без дополнительных параметров Docker приложение будет видеть окружение контейнера
и доступные ему представления `/proc` и `/sys`.

### 3. Запуск через Docker Compose

Соберите и запустите сервис одной командой:

```bash
docker compose up --build -d
```

Остановка сервиса:

```bash
docker compose down
```

После старта интерфейс доступен по адресу:

```text
http://localhost:8080
```

## Порты и API

| Порт | Назначение |
| ---- | ---------- |
| `8080` | HTTP-сервер, WebUI и REST endpoint `/api/stats` |
| `8081` | WebSocket-сервер для live-обновлений |

REST endpoint:

```text
GET http://localhost:8080/api/stats
```

WebSocket endpoint:

```text
ws://localhost:8081
```

Формат live-сообщения WebSocket:

```json
{
  "cpu": 12.5,
  "ram_usage_perc": 47.3,
  "ram_used_gb": 7.6,
  "ram_total_gb": 16.0,
  "load_avg": [0.42, 0.51, 0.63],
  "temps": {
    "x86_pkg_temp": 52.0
  },
  "procs": [
    {
      "pid": 1234,
      "name": "firefox",
      "memUsageMB": 512.4
    }
  ]
}
```

## Ручная сборка без `run.sh`

Backend:

```bash
mkdir -p backend/build
cd backend/build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j"$(nproc)"
cd ../..
```

Frontend:

```bash
mkdir -p public/src
cd frontend
npm install
npx tsc --outDir ../public/src/
cd ..
cp frontend/*.html public/
cp frontend/*.css public/
```

Запуск:

```bash
./backend/build/monitor_backend
```

## Особенности и ограничения

- Приложение рассчитано на Linux, использует `/proc` и `/sys`.
- Температуры отображаются только при наличии файлов thermal zones в
  `/sys/class/thermal` и прав на их чтение.
- В Docker без дополнительных настроек данные относятся к среде контейнера и
  доступным контейнеру системным представлениям.
- Frontend получает live-данные через WebSocket, поэтому для корректной работы
  должны быть доступны оба порта: `8080` и `8081`.