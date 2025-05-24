# HTTP Log Analyzer

Программа для анализа логов HTTP-сервера (форматов CLF, Combined и произвольных custom) с использованием регулярных выражений. Поддерживает многопоточную обработку, конфигурацию через JSON и расширяемую аналитику.

---

## 📦 Зависимости

Ubuntu:

```bash
sudo apt update
sudo apt install g++ cmake make libnlohmann-json-dev
```

Если используешь `boost::regex` (для поддержки именованных групп):

```bash
sudo apt install libboost-regex-dev
```

---

## ⚙️ Сборка

```bash
mkdir build
cd build
cmake ..
make
```

---

## 📂 Структура проекта

```
http_log_analyzer/
├── main.cpp
├── CMakeLists.txt
├── logs/
│   ├── access_combined.log
│   ├── access_clf.log
│   └── custom_config.json
└── build/
```

---

## 🧾 Пример конфигурационного JSON (для кастомного формата)

```json
{
  "log_format": "custom",
  "regex": "^(\\S+) - - \\[([^\\]]+)\\] \\\"(\\S+) (\\S+) HTTP/\\S+\\\" (\\d{3}) (\\d+) \\\"([^\\\"]*)\\\" \\\"([^\\\"]*)\\\"$",
  "fields": {
    "ip": "1",
    "datetime": "2",
    "method": "3",
    "url": "4",
    "code": "5",
    "size": "6",
    "referer": "7",
    "useragent": "8"
  }
}
```

---

## 🚀 Примеры запуска

### Combined формат

```bash
./log_analyzer \
  --logfile ../logs/access_combined.log \
  --format combined \
  --topip 10 \
  --topurl 5 \
  --time-stats \
  --ip-filter 192.168.1.100 \
  --url-filter /index.html
```

### Common формат

```bash
./log_analyzer \
  --logfile ../logs/access_clf.log \
  --format common \
  --topip 10 \
  --topurl 5 \
  --time-stats \
  --ip-filter 192.168.1.100 \
  --url-filter /index.html
```

### Custom формат (через JSON)

```bash
./log_analyzer \
  --logfile ../logs/access_combined.log \
  --config ../logs/custom_config.json \
  --topip 10 \
  --topurl 5 \
  --time-stats \
  --ip-filter 192.168.1.100 \
  --url-filter /index.html \
  --start "2023-11-10 10:00:00" \
  --end "2023-11-10 10:05:00"
```

---

## 🔍 Поддерживаемая аналитика

- Топ N IP-адресов `--topip`
- Топ N URL `--topurl`
- Топ N User-Agent `--topua`
- Распределение по HTTP-кодам
- Статистика по часам (`--time-stats`)
- Фильтрация по IP (`--ip-filter`) и URL (`--url-filter`)
- Фильтрация по времени (`--start`, `--end`) — формат: `"YYYY-MM-DD HH:MM:SS"`

---

