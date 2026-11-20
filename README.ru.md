# ESP32OT

[English](README.md)

[Полная справка API](docs/API.ru.md)

Библиотека OpenTherm для ESP32 с аппаратными TX/RX через RMT и внутренним
FreeRTOS worker. Поддерживает master/slave, типизированные сообщения,
синхронные и асинхронные вызовы, прозрачный прокси с двумя интерфейсами.

## Благодарности

**ESP32OT существует благодаря работе Игоря Мельника и его оригинальной
[библиотеке OpenTherm](https://github.com/ihormelnyk/opentherm_library).**
Его реализация стала основой, из которой развился этот проект.

ESP32OT разработана в рамках работы над
[OTGateway](https://github.com/Laxilef/OTGateway). Репозиторий отдельного
проекта — [Laxilef/ESP32OT](https://github.com/Laxilef/ESP32OT).
Библиотека может использоваться независимо от OTGateway.

## Требования

- Arduino-ESP32 и C++17 или новее.
- RMT API ESP-IDF 6. Конфигурация разработки использует ветку pioarduino
  `prep_IDF6`; старые SDK не являются поддерживаемой заменой.
- ESP32, ESP32-S2, ESP32-S3, ESP32-C3 или ESP32-C6 со свободными RMT-ресурсами.
- Электрический OpenTherm-интерфейс для каждой шины. Подключать GPIO ESP32
  непосредственно к линии OpenTherm нельзя.

## Установка и проект примеров

Публичный заголовок — `ESP32OT.h`, класс — `ESP32OT::ESP32OT`.
Локальную копию можно поместить в каталог
`lib/ESP32OT` своего PlatformIO-проекта либо установить как Arduino-библиотеку.
Для ESP-IDF-проекта с Arduino-компонентом предусмотрен [CMakeLists.txt](CMakeLists.txt).

Корневой [platformio.ini](platformio.ini) собирает выбранный пример как приложение
и подключает ESP32OT как отдельную библиотеку. Команды выполняются из корня
библиотеки:

```sh
pio run
pio run -e esp32-c3
pio run -e esp32-c6
```

По умолчанию выбраны `esp32-s3` и `examples/Master`. Также доступны окружения
`esp32`, `esp32-s2`, `esp32-c3`, `esp32-c6` и `ci`.
Выберите одну строку `src_dir` в конфигурации либо задайте абсолютный путь
через переменную окружения `PLATFORMIO_SRC_DIR`. Например, в PowerShell:

```powershell
$env:PLATFORMIO_SRC_DIR = (Resolve-Path examples/Slave).Path
pio run -e esp32-s3
Remove-Item Env:PLATFORMIO_SRC_DIR
```

Окружение `ci` принимает board ID через `PIO_BOARD`: сборка — `pio run -e ci`.
`lib_deps = ESP32OT=symlink://.` подключает текущую копию через `library.json`
и собирает все вложенные исходники `src/`. `lib_dir = .pio/lib` ограничивает
поиск этим проектом. Выбор примера не меняет исходники библиотеки.
Профиль S3 использует USB-Serial/JTAG и watchdog-reset после загрузки.
Master-примеры, включая `MasterHeap` и `ReadAll`, используют `MASTER_RX_GPIO` /
`MASTER_TX_GPIO`; профиль S3 задаёт RX=35 / TX=36, резервные значения sketch — 4 / 5.
`MASTER_*_GPIO` задаёт master-интерфейс ESP32, подключённый к котлу;
`SLAVE_RX_GPIO` / `SLAVE_TX_GPIO` — slave-интерфейс, подключённый к термостату.
У `Slave` по умолчанию GPIO4/5; у slave-интерфейса прокси — GPIO6/7, либо
GPIO18/19 на обычном ESP32.
Конфигурация сохраняет стандарт C++ SDK и не зависит от настроек, секретов или
скриптов OTGateway. Используемая ветка платформы предназначена для разработки,
а не является стабильным релизом SDK.

## Быстрый старт

```cpp
#include <ESP32OT.h>

static ESP32OT::ESP32OT boiler(GPIO_NUM_4, GPIO_NUM_5);

void setup() {
  Serial.begin(115200);
}

void loop() {
  // ESP32OT initialization does not prove that a boiler is connected.
  if (!boiler.isInitialized()) {
    if (!boiler.begin()) {
      delay(1000);
      return;
    }
  }

  // Status uses ID0. Default master flags do not request heating.
  auto status = boiler.sendRequest(ESP32OT::Messages::Status());
  if (status.isSuccess()) {
    Serial.printf("Flame: %u\n", static_cast<unsigned int>(status.isFlameOn()));
  }

  delay(750);
}
```

Выберите GPIO типа `gpio_num_t` по своей плате и интерфейсу.
`begin()` запускает worker/RMT; наличие отвечающего устройства определяется
отдельно. Повторный `begin()` активного экземпляра возвращает `false` до успешного
`end()`. `isReady()` означает доступность шины с учётом межкадровой паузы,
а не появление нового непрочитанного ответа.

## Конфигурация и жизненный цикл

```cpp
ESP32OT::ESP32OT::Config config;
config.rxGpio = GPIO_NUM_4;
config.txGpio = GPIO_NUM_5;
config.isSlave = false;
config.alwaysReceive = false;
config.maxAttempts = 5;

ESP32OT::ESP32OT boiler(config);
```

Позиционный конструктор также сохранён:

```cpp
ESP32OT::ESP32OT boiler(GPIO_NUM_4, GPIO_NUM_5, false, false);
```

| Поле | По умолчанию | Назначение |
| --- | --- | --- |
| `rxGpio`, `txGpio` | `GPIO_NUM_NC` | GPIO электрического интерфейса |
| `isSlave` | `false` | Роль master; `true` выбирает slave |
| `alwaysReceive` | `false` | Дополнительный приём master между запросами |
| `maxAttempts` | `5` | Все попытки sync master, включая первую; 0 означает одну |
| `stackSize` | `3072` байт | Начальный размер стека worker |
| `priority` | `5` | Приоритет worker FreeRTOS |
| `core` | `0` | Ядро worker и владельца RMT |
| `acceptInvalidRequests` | `false` | Привязка полных невалидных slave-запросов к sequence |

Slave слушает линию между ответами независимо от `alwaysReceive`.
Master с `alwaysReceive=true` также сообщает незапрошенные полные кадры
через `onResponse()`.

`getConfig()` возвращает `const Config&`. Меняйте копию и передавайте её
в `setConfig()` остановленного экземпляра:

```cpp
auto config = boiler.getConfig();
config.maxAttempts = 3;
boiler.setConfig(config);
```

`setConfig()` заменяет всю конфигурацию. Смена роли очищает оба обработчика
запросов и ответов: регистрируйте их после выбора роли. Status callback
сохраняется. `begin(rxGpio, txGpio)` обновляет сохранённые GPIO
только после успешного запуска. Настройку и жизненный цикл одного экземпляра
нужно выполнять последовательно.

`end()` кооперативно завершает worker и удаляет TX прежде RX. При ошибке очистки
ресурсы сохраняют владельца, и `end()` можно повторить. После фатальной ошибки
`isInitialized()` может быть false при ещё не освобождённых ресурсах;
одного повторного `begin()` недостаточно. Деструктор аварийно завершает работу,
если не может остановить worker, чтобы тот не использовал уничтоженные данные.

`getLastError()` возвращает ошибку SDK. `getWorkerStackHighWaterMark()` показывает
наименьший оставшийся запас стека в байтах. Начальные 3072 байта не являются
измеренным безопасным минимумом для любых callbacks.

## Сообщения и payload

`ESP32OT::Messages::Message` хранит raw-кадр и metadata операции.
`build()` возвращает кадр, `parse(snapshot)` заменяет снимок. Производные
обёртки разбирают поля на лету, без второй копии payload и heap на сообщение.

Все типы доступны через `ESP32OT.h`. Короткие имена enum:
`ESP32OT::MessageID`, `ESP32OT::MessageType`, `ESP32OT::OperationStatus`.

```cpp
// A value selects WRITE_DATA; the ID-only constructor selects READ_DATA.
auto setpoint = boiler.sendRequest(
  ESP32OT::Messages::Float(ESP32OT::MessageID::TSet, 55.0f)
);
auto temperature = boiler.sendRequest(
  ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler)
);
if (temperature.isSuccess()) {
  float value = temperature.get();
}
```

Типизированный sync-вызов возвращает ту же обёртку. Передача через базовый
`const Message&` возвращает `Message`. Для принятого raw-сообщения формат
выбирается явно:

```cpp
auto config = request.as<ESP32OT::Messages::SlaveConfig>();
bool dhwPresent = config.isDhwPresent();
```

`as<T>()` создаёт самостоятельную копию без RTTI и heap. Перед чтением полей
нужно проверить статус операции. Ответ на активный master-запрос обязан иметь
тот же ID; другой ID даёт `INVALID`. Незапрошенные кадры passive RX не имеют
текущего запроса для сопоставления: обёртку выбирают по фактически принятому ID.

| Обёртки `ESP32OT::Messages` | Поля / ID |
| --- | --- |
| `Float`, `UInt8`, `UInt16`, `Int16` | Signed f8.8, u8 в LB, u16, s16 |
| `Status` | Запрошенные master flags и фактические slave flags, ID0 |
| `MasterConfig`, `SlaveConfig` | Возможности и member ID, ID2/3 |
| `RemoteRequest`, `Fault`, `RemoteParameters` | Команда/результат, неисправности, поддержка и запись, ID4/5/6 |
| `Capacity`, `FanSpeed`, `TemperatureBounds` | Мощность/модуляция, скорости в Гц, signed-границы, ID15/35/48/49 |
| `ParameterCount`, `IndexedValue` | Размеры и записи TSP/fault history |
| `ProductVersion`, `BrandCharacter` | Тип/версия продукта и индексированные ASCII-символы |
| `Date`, `DayTime` | Месяц/день и день недели/часы/минуты, ID21/20 |
| `VentilationStatus`, `VentilationFault`, `VentilationConfig` | Флаги и данные вентиляции, ID70/72/74 |
| `VentilationRemoteParameters`, `NominalVentilation` | Поддержка/запись и номинальная вентиляция в HB, ID86/87 |
| `RFSensorStatus`, `RemoteOverrideMode` | RF/батарея/сигнал и отдельные CH/DHW режимы, ID98/99 |
| `RemoteOverrideFunction` | Приоритет ручного/программного изменения, ID100 |
| `SolarStorageStatus`, `SolarStorageFault`, `SolarStorageConfig` | Режим/состояние, OEM-код, конфигурация/member ID, ID101/102/103 |

Числовые getters: `Float::get()`, `UInt8::get()`, `UInt16::getUInt()`,
`Int16::getInt()`. `Float` поддерживает signed f8.8 от -128 до 127,99609375,
дробная часть кодируется с усечением. Отдельный `Float::writeTemperature()`
сохраняет прежний clamp 0..100; обычный `Float` его не применяет.

Структуры имеют именованные поля и setters флагов, как `Status`.
`SlaveConfig::isDhwPresent()` означает наличие DHW,
`RemoteParameters::isDhwSetpointWritable()` — разрешение записи уставки.
Поддержка и право записи независимы. Setters сохраняют остальные биты.

`reply()` собирает ответ с сохранением ID и привязки к исходному запросу:

```cpp
// In the ID3 READ_DATA branch of a slave request handler.
auto requestConfig = request.as<ESP32OT::Messages::SlaveConfig>();
auto response = requestConfig.reply(ESP32OT::MessageType::READ_ACK, 0, 42);
response.setDhwPresent(true).setCh2Present(true);
slave.sendResponseAsync(response);
```

`IndexedValue(id, index)` формирует чтение, третий аргумент со значением — запись.
`BrandCharacter` учитывает особый ACK: в запросе HB — индекс символа, в ответе —
максимальный индекс. `getRequestIndex()` использует сохранённый запрос,
`getMaxIndex()` — ответ. Строка целиком не собирается и память под неё не выделяется.

### Совместимость

Форматы сверены с предоставленной спецификацией OpenTherm v4.2.
Существующие допуски для разных устройств сохраняются. Обёртки кодируют ширину
полей и сохраняют принятые raw-биты; прикладные диапазоны проверяет вызывающий код.

- ID30: подробная таблица говорит s16, сводка — f8.8. Прежнее поведение OTGateway
  использует f8.8; для его сохранения выбирайте `Float`.
- ID100: `RemoteOverrideFunction` использует LB bits0/1 по подробной таблице;
  сводное описание `flag8/-` с ней расходится.
- ID103: таблица configuration/member ID противоречит описанию system-type bit.
  `SolarStorageConfig` предоставляет байт конфигурации и member ID,
  не угадывая значение неоднозначного бита.
- ID99: режим 2 для CH — comfort, для DHW — anti-legionella; enum разделены.
  Ноль означает отсутствие override, OFF — 6.
- Ноль допустим для ряда полей, включая ID56 и день недели 0 (неизвестен).
  Общего запрета нулевой температуры нет.

## Синхронные и асинхронные операции

`sendRequest()` ждёт результат и повторяет master-результаты `INVALID`, таймауты
и восстанавливаемые ошибки до `Config::maxAttempts`, включая первую попытку.
`SUCCESS`, валидные ответы `DATA_INVALID` или `UNKNOWN_DATA_ID`, отмена или
остановка прекращают повторы. Неудачные попытки остаются доступны в callbacks,
даже если следующая попытка успешна. `sendResponse()` один раз передаёт
slave-ответ и ждёт TX_DONE; подтверждение от master не ожидается.

Вызывающая задача уступает CPU шагами по 10 мс. Пользовательский обработчик
ожидания принимает указатель функции и context:

```cpp
using DelayCallback = void (*)(unsigned int milliseconds, void* context);
ESP32OT& setDelayCallback(DelayCallback callback = nullptr, void* context = nullptr);
```

Задавать или удалять его можно только у остановленного экземпляра: до `begin()`
или после успешного `end()`. Изменения во время работы игнорируются. Фасад
хранит два указателя, не выделяя память и не владея хранилищем context.
Вызывающий код должен сохранять это хранилище действительным во время вызовов.
`setDelayCallback()` без аргументов возвращает стандартный `vTaskDelay()` с
минимумом один tick. RMT работает независимо. Delay callback предназначен
только для ожидания/уступки CPU: повторный sync-вызов того же ESP32OT из него
не поддерживается.

`sendRequestAsync()` и `sendResponseAsync()` копируют сообщение и сразу
возвращают управление. `true` означает принятую команду, а не успешную передачу.
Async делает одну попытку. На экземпляр приходится одна ожидающая/исполняемая
операция; новая команда отклоняется, пока слот занят.

Результат доступен через самостоятельный снимок `getResult()` / `getResult<T>()`,
через заполнение существующего `getResult(destination)` или callbacks.
`getRequest()` возвращает последний slave-запрос. Внешний `process()` не нужен.

| Callback | Роль master | Роль slave |
| --- | --- | --- |
| `onStatusChange()` | Переходы публичного статуса, включая ошибки без полного кадра | Переходы публичного статуса, включая ошибки без полного кадра |
| `onRequest()` | Исходящий запрос перед попыткой локальной передачи | Полный принятый запрос; обычный валидный запрос имеет `REQUEST_RECEIVED` |
| `onResponse()` | Полный принятый ответ, включая невалидные/отрицательные | Исходящий ответ перед попыткой локальной передачи |

`MessageCallback` и `StatusCallback` — одноаргументные обработчики
`std::function`, поддерживающие захватывающие лямбды. Например, на стороне master:

```cpp
const char* interfaceName = "boiler";
boiler.onRequest([interfaceName](const ESP32OT::Messages::Message& request) {
  Serial.printf(
    "%s TX=%08lX\n",
    interfaceName,
    static_cast<unsigned long>(request.getRaw())
  );
});
boiler.onResponse([interfaceName](const ESP32OT::Messages::Message& response) {
  Serial.printf(
    "%s RX=%08lX\n",
    interfaceName,
    static_cast<unsigned long>(response.getRaw())
  );
});
```

Исходящие уведомления имеют `TRANSMITTING` и `hasFrame() == false`; они возникают
один раз для каждой попытки до обращения к драйверу. Последующая отмена
или ошибка ещё может предотвратить TX. Это не завершение TX и не доставка.
Входящие уведомления по-прежнему
гарантируют полный принятый кадр; успешность проверяется отдельно.

Прежняя регистрация указателя функции с context поддерживается перегрузками
`LegacyMessageCallback` и `LegacyStatusCallback`. Вызов setter без аргументов
или с `nullptr` снимает hook. `DelayCallback` сохраняет сигнатуру указателя
функции с context и настройку только при остановленном экземпляре.

Callbacks выполняются worker-ом вне ISR и внутренних locks. Ссылка на сообщение
действует только внутри вызова; сохраните копию, если данные нужны позже.
Sync send и lifecycle-вызовы отклоняются из worker того же экземпляра.
Асинхронная пересылка другому экземпляру поддерживается.
Регистрация непустого обработчика выделяет память для совместно владеемого
callable; захваты могут выделять дополнительную память. При вызове сохраняется
совместное владение, а сам callable не копируется. Замыкание принадлежит
библиотеке, но объекты по ссылкам, `this` и legacy context принадлежат вызывающему.
Замена/удаление hook не ждёт завершения уже начатого или подготовленного вызова:
захваченные ссылки и context должны оставаться действительными до его завершения.
Для гарантированной замены сначала успешно остановите экземпляр, затем замените
hook перед повторным запуском. TX-обработчики должны быть короткими: их работа
задерживает начало передачи.

`isComplete()` проверяет терминальный статус, `hasFrame()` — наличие полного
принятого кадра. Таймаут или неполный приём могут завершиться без него;
`getRaw()` тогда нельзя выдавать за полный ответ. Полный кадр тоже бывает
невалидным: `isSuccess()` проверяется отдельно. У успешного slave TX `hasFrame()`
остаётся false: это завершение передачи, не подтверждение приёма.

Терминальные статусы — `SUCCESS`, `DATA_INVALID`, `UNKNOWN_DATA_ID`, `INVALID`,
`TIMEOUT`, `ERROR` и `CANCELLED`. Корректно принятый отрицательный ответ имеет
собственный статус: `DATA_INVALID` означает, что запрошенные данные недоступны
или операция не принята; `UNKNOWN_DATA_ID` — ID не поддерживается. Это валидные
ответы протокола: `isComplete()` и `hasFrame()` возвращают true, а `isSuccess()` —
false. Проверяйте `isSuccess()` перед интерпретацией payload как запрошенных
данных. Ошибки структуры кадра, чётности, типа ответа или несовпадение ID
активного запроса дают `INVALID`.
`INVALID` завершает отдельную попытку; синхронный master-вызов может повторить
её, пока остаются попытки. Асинхронная операция делает только одну попытку.

Metadata включает статус, ошибку SDK, номер попытки/операции/запроса, raw-запрос
и ответ, `elapsedUs` одной попытки и `totalElapsedUs` sync-вызова.
Slave-ответ обязан соответствовать текущему sequence: ответ старому запросу
или прежней сессии после end/begin отклоняется.

## Примеры

Подробности — в [русском руководстве примеров](examples/README.ru.md).

- [Master](examples/Master/Master.ino): sync-ответы обрабатываются в `loop()`.
- [MasterCallbacks](examples/MasterCallbacks/MasterCallbacks.ino): sync-отправка,
  завершения попыток обрабатываются callback.
- [MasterAsync](examples/MasterAsync/MasterAsync.ino): async Status, штатный
  `isReady()` и минимальный прикладной интервал 750 мс.
- [MasterHeap](examples/MasterHeap/README.ru.md): замеры heap/стека и свободных
  блоков, проверка фрагментации, жизненного цикла master и всех обёрток.
- [ReadAll](examples/ReadAll/README.ru.md): последовательное чтение всех 87 ID,
  доступных для чтения, включая все заявленные TSP и записи истории ошибок;
  вывод статуса ответа и успешных данных из `loop()`.
- [TestAllIDs](examples/TestAllIDs/README.ru.md): последовательная проверка ID,
  индексированных данных, карта наблюдаемого доступа и статистика статусов;
  проверки записи требуют явной настройки и по умолчанию отключены.
- [Slave](examples/Slave/Slave.ino): небольшая модель ведомого с явными ответами.
- [TransparentProxy](examples/TransparentProxy/TransparentProxy.ino): raw-пересылка
  между двумя интерфейсами — термостатом и котлом.

Прокси сохраняет raw-биты, включая полные невалидные кадры, и metadata
маршрутизации. На slave-стороне включён `acceptInvalidRequests=true`.
Новый запрос термостата может заменить старый; устаревший ответ котла отклоняется.
Выдуманные ответы и собственные прикладные повторы не добавляются.

Для понятности examples печатают в Serial прямо из callbacks. Это не является
доказательством выполнения требования пересылки gateway за 7 мс.

## Ресурсы RMT и тайминги

Экземпляр владеет закреплённым за ядром worker, TX-каналом, RX-каналом и двумя
фиксированными RX-буферами по 64 символа. RMT IRQ выделяются на ядре владельца.
RX включается из TX_DONE, не дожидаясь планирования задачи. Прикладных GPIO ISR
на каждый фронт и глобального отключения прерываний нет.

| SoC | Аппаратные TX / RX символы на экземпляр | Два интерфейса |
| --- | --- | --- |
| ESP32 | 64 / 64 | Автоматическое выделение |
| ESP32-S2 | 64 / 64 | Автоматическое выделение |
| ESP32-S3 | 48 / 96 | Два RX-блока на интерфейс |
| ESP32-C3 | 48 / 48 | Полный capture собирает RX ping-pong SDK |
| ESP32-C6 | 48 / 48 | Полный capture собирает RX ping-pong SDK |

На каждую операцию heap не выделяется; память worker/драйвера
создаётся при запуске. Расход стека callbacks проверяют в реальном приложении.

TX: start + 32 бита MSB-first + stop, половины по 500 мкс. Допуски RX
start/sample/stop и перенос частичного состояния между captures сохранены.
RX поддерживает асимметричные половины бита. Если полный capture требует
восстановления фазы, принимается только однозначная Manchester-интерпретация
по времени импульсов. Проверки чётности, типа ответа и ID активного запроса
выполняются после этого выбора; ожидаемый ID или чётность не используются
для выбора декодирования, чётность не исправляется. Неоднозначное восстановление
остаётся `INVALID`; синхронный master-вызов может повторить попытку, пока они
не исчерпаны.

Receive inactivity deadline — **1 042 500 мкс**: 1 с + 40 мс запаса на кадр
и 2,5 мс capture idle. MWT master — 100 мс, slave — 20 мс от RX_DONE;
TX watchdog — 1 с. Готовый capture обрабатывается до решения о таймауте.
Точность timestamp и задержка пробуждения worker — разные величины.

По §4.3.2 OpenTherm v4.2 пересылка gateway должна начинаться за 7 мс.
Idle RMT 2,5 мс уже занимает часть бюджета. Для заявления о соответствии
нужны измерения обоих интерфейсов с учётом планировщика и cache.

## Разработка и проверки

[AGENTS.md](AGENTS.md) содержит правила разработки и важные решения по протоколу
и архитектуре. Английский [README.md](README.md) и этот файл должны оставаться
согласованными; комментарии к коду пишутся на английском.

## Лицензия

ESP32OT распространяется под [GPL-3.0-only](LICENSE).
