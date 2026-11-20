# ESP32OT: полная справка API и реализации

[English](API.md) · [Обзор проекта](../README.ru.md) · [Примеры](../examples/README.ru.md)

Справка описывает объявления и поведение текущих исходников: фасад приложения,
все wrapper сообщений, протокольные типы, конфигурацию, callbacks, методы
транспорта/драйвера и внутренние вспомогательные типы. Закрытые члены приведены
для сопровождения реализации; приложение обычно включает только `<ESP32OT.h>`
и использует `ESP32OT::ESP32OT` и `ESP32OT::Messages`.

ESP32OT — независимая master/slave-библиотека OpenTherm для ESP32, разработанная
при работе над [OTGateway](https://github.com/Laxilef/OTGateway) на основе
[библиотеки Ihor Melnyk](https://github.com/ihormelnyk/opentherm_library).
Библиотека распространяется по GPL-3.0-only: [LICENSE](../LICENSE).
Благодарность автору исходной работы сохранена в документации. Требуются Arduino-ESP32 с RMT API ESP-IDF6
и C++17 или новее. Профили плат: ESP32, S2, S3, C3 и C6. GPIO и внешний
OpenTherm-адаптер выбираются под плату и приложение.

## Содержание

- [Как пользоваться справкой](#как-пользоваться-справкой)
- [Устройство и владение ресурсами](#устройство-и-владение-ресурсами)
- [Полный пример master](#полный-пример-master)
- [Асинхронное завершение и владение callback](#асинхронное-завершение-и-владение-callback)
- [Завершение, повторы и время](#завершение-повторы-и-время)
- [Значения, raw и маршрутизация](#значения-raw-и-маршрутизация)
- [Выбор примеров](#выбор-примеров)
- [Публичный facade: `ESP32OT::ESP32OT`](#публичный-facade-esp32otesp32ot)
  - [Aliases типов facade](#aliases-типов-facade)
  - [Методы и специальные функции класса](#методы-и-специальные-функции-класса)
  - [Приватные поля](#приватные-поля)
- [Модель сообщений и полный справочник wrappers](#модель-сообщений-и-полный-справочник-wrappers)
  - [Messages::Message](#messagesmessage)
  - [Messages::BrandCharacter](#messagesbrandcharacter)
  - [Messages::Capacity](#messagescapacity)
  - [Messages::Date](#messagesdate)
  - [Messages::DayTime](#messagesdaytime)
  - [Messages::FanSpeed](#messagesfanspeed)
  - [Messages::Fault](#messagesfault)
  - [Messages::Float](#messagesfloat)
  - [Messages::IndexedValue](#messagesindexedvalue)
  - [Messages::Int16](#messagesint16)
  - [Messages::MasterConfig](#messagesmasterconfig)
  - [Messages::NominalVentilation](#messagesnominalventilation)
  - [Messages::ParameterCount](#messagesparametercount)
  - [Messages::ProductVersion](#messagesproductversion)
  - [Messages::RFSensorStatus](#messagesrfsensorstatus)
  - [Messages::RemoteOverrideFunction](#messagesremoteoverridefunction)
  - [Messages::RemoteOverrideMode](#messagesremoteoverridemode)
  - [Messages::RemoteParameters](#messagesremoteparameters)
  - [Messages::RemoteRequest](#messagesremoterequest)
  - [Messages::SlaveConfig](#messagesslaveconfig)
  - [Messages::SolarStorageConfig](#messagessolarstorageconfig)
  - [Messages::SolarStorageFault](#messagessolarstoragefault)
  - [Messages::SolarStorageStatus](#messagessolarstoragestatus)
  - [Messages::Status](#messagesstatus)
  - [Messages::TemperatureBounds](#messagestemperaturebounds)
  - [Messages::UInt16](#messagesuint16)
  - [Messages::UInt8](#messagesuint8)
  - [Messages::VentilationConfig](#messagesventilationconfig)
  - [Messages::VentilationFault](#messagesventilationfault)
  - [Messages::VentilationRemoteParameters](#messagesventilationremoteparameters)
  - [Messages::VentilationStatus](#messagesventilationstatus)
- [Типы протокола и сырые кадры](#типы-протокола-и-сырые-кадры)
  - [`ESP32OT::Protocol::MessageType`](#esp32otprotocolmessagetype)
  - [`ESP32OT::Protocol::MessageID`](#esp32otprotocolmessageid)
  - [`ESP32OT::Protocol::OperationStatus`](#esp32otprotocoloperationstatus)
  - [`ESP32OT::MessageID / MessageType / OperationStatus`](#esp32otmessageid--messagetype--operationstatus)
  - [`ESP32OT::Protocol::MessageData`](#esp32otprotocolmessagedata)
  - [`ESP32OT::Protocol::Frame`](#esp32otprotocolframe)
- [Конфигурация и callbacks транспорта](#конфигурация-и-callbacks-транспорта)
  - [`ESP32OT::Transport::Config`](#esp32ottransportconfig)
  - [`ESP32OT::Transport::StatusEvent`](#esp32ottransportstatusevent)
  - [`ESP32OT::Transport::StatusCallback / MessageCallback`](#esp32ottransportstatuscallback--messagecallback)
  - [`ESP32OT::Transport::LegacyStatusCallback / LegacyMessageCallback`](#esp32ottransportlegacystatuscallback--legacymessagecallback)
  - [`ESP32OT::Transport::RmtTransport`](#esp32ottransportrmttransport)
  - [Внутреннее устройство RmtTransport](#внутреннее-устройство-rmttransport)
  - [`ESP32OT::Transport::RmtTransport::ActiveOperation`](#esp32ottransportrmttransportactiveoperation)
  - [Приватные методы RmtTransport](#приватные-методы-rmttransport)
- [Справочник внутреннего RMT-драйвера](#справочник-внутреннего-rmt-драйвера)
  - [`ESP32OT::Transport::RmtDriver`](#esp32ottransportrmtdriver)
  - [`ESP32OT::Transport::RmtDriver::State`](#esp32ottransportrmtdriverstate)
  - [`ESP32OT::Transport::RmtDriver::ResultStatus`](#esp32ottransportrmtdriverresultstatus)
  - [Публичные методы нижнего уровня RmtDriver](#публичные-методы-нижнего-уровня-rmtdriver)
  - [Приватные методы RmtDriver](#приватные-методы-rmtdriver)
  - [Типы, константы и функции декодирования внутри CPP](#типы-константы-и-функции-декодирования-внутри-cpp)
  - [`ESP32OT::Transport::RmtEventType`](#esp32ottransportrmteventtype)
  - [`ESP32OT::Transport::RmtEvent`](#esp32ottransportrmtevent)
  - [`ESP32OT::Transport::RmtDriver::RmtContext`](#esp32ottransportrmtdriverrmtcontext)
  - [`ESP32OT::Transport::RmtDecodedFrame`](#esp32ottransportrmtdecodedframe)
  - [`ESP32OT::Transport::RmtPulse`](#esp32ottransportrmtpulse)
  - [`ESP32OT::Transport::RmtPhaseCandidate`](#esp32ottransportrmtphasecandidate)

## Как пользоваться справкой

- **API приложения:** фасад и классы сообщений. Непубличные методы отмечены
  явно. Конструкторы/фабрики создают снимки значений.
- **Протокольный API:** enum, raw-frame helpers и metadata — для неизвестных ID,
  полей производителя, логов и точной пересылки кадров.
- **Реализация транспорта:** worker и RMT-драйвер. Их публичные C++ методы —
  интерфейс нижнего уровня, а не дополнительный обязательный цикл приложения.
- **Внутренний пример:** вызов внутри владельца/вспомогательной функции является
  контекстным псевдокодом. Он не делает private-метод доступным приложению.
- Каждая явно объявленная перегрузка имеет отдельную запись. Унаследованные
  методы описаны у объявляющего класса и применимы к производным wrappers.
  Неявные copy constructor/assignment wrappers копируют обычное значение.

Заголовок `<ESP32OT.h>` включает фасад и все wrappers сообщений. Пространство
`ESP32OT` предоставляет aliases `MessageID`, `MessageType`, `OperationStatus`
для enum из `ESP32OT::Protocol`. Вложенные aliases фасада обозначают те же
типы; отдельного формата кадра или выделения памяти у них нет.

## Устройство и владение ресурсами

```mermaid
flowchart LR
  App[Задача приложения] --> Facade[Фасад ESP32OT]
  Facade --> Transport[RmtTransport: снимки и команды]
  Transport --> Worker[Закреплённый FreeRTOS worker]
  Worker --> Driver[RmtDriver]
  Driver --> RMT[ESP32 RMT TX и RX]
  RMT --> Adapter[OpenTherm-адаптер]
  Adapter --> Peer[Котёл или термостат]
  RMT --> Events[Очередь событий ISR и фиксированные буферы]
  Events --> Worker
```

Каждый контроллер владеет отдельным worker, состоянием транспорта и RMT-ресурсами.
Worker закреплён на `Config::core`, выделяет и освобождает ресурсы на этом ядре,
обрабатывает завершённые захваты до проверки таймаутов и вызывает callbacks вне
ISR и внутренних блокировок. На экземпляр приходится одна ожидающая/выполняемая
операция. Приложение не вызывает цикл `process()`. Шлюз использует два независимых
контроллера со своими GPIO, ролями и временными ограничениями.

Синхронный вызов ожидает с передачей CPU; worker продолжает работать независимо.
Асинхронный сразу возвращает принятие команды, а результат появляется позже
в снимке/callback. Принятие команды и `isReady()` не доказывают подключение
устройства. Инициализация, готовность шины, завершение операции, полный кадр
и подтверждённые данные — разные условия.

## Полный пример master

```cpp
#include <ESP32OT.h>

static ESP32OT::ESP32OT controller;

void setup() {
  Serial.begin(115200);
  ESP32OT::ESP32OT::Config config;
  config.rxGpio = GPIO_NUM_35;
  config.txGpio = GPIO_NUM_36;
  config.maxAttempts = 5;
  controller.setConfig(config);
}

void loop() {
  if (!controller.isInitialized()) {
    if (!controller.end() || !controller.begin()) {
      Serial.printf("begin error=%ld\n", static_cast<long>(controller.getLastError()));
      delay(1000);
      return;
    }
  }

  const auto result = controller.sendRequest(
    ESP32OT::Messages::Float::read(ESP32OT::MessageID::Tboiler)
  );
  Serial.println(ESP32OT::Protocol::Frame::statusToString(result.getStatus()));
  if (result.isSuccess()) {
    Serial.printf("boiler temperature=%.2f C\n", result.get());
  }

  if (result.hasFrame()) {
    Serial.printf("raw=%08lX\n", static_cast<unsigned long>(result.getRaw()));
  }

  delay(150);
}
```

GPIO35/36 приведены как пример и не являются универсальным выбором для всех плат.
`Float::read(Tboiler)` возвращает wrapper, а типизированный `sendRequest` сохраняет
его тип в результате. Только `isSuccess()` разрешает использовать payload как
подтверждённые данные. `hasFrame()` нужен при выводе результата timeout/error,
где принятого кадра может не быть; он не заменяет проверку корректности/успеха.

Для Status создайте `Messages::Status` и задайте нужные master flags.
Запрошенное разрешение CH/DHW и фактическая активность slave — разные поля.
Нулевая температура не является универсальной командой отключения отопления.
ReadAll читает диагностические данные и Status с очищенными master enable flags;
выбирайте их с учётом приложения. Библиотека не назначает порядок опроса ID.

## Асинхронное завершение и владение callback

Наблюдатели сообщений/статусов принимают одноаргументные `std::function`, включая
захватывающие лямбды. Этот master-пример независимо наблюдает исходящие запросы
и входящие ответы; захваченный label указывает на строковый литерал, который
остаётся действительным после выхода из setup.

```cpp
using Controller = ESP32OT::ESP32OT;
using Message = ESP32OT::Messages::Message;

static Controller controller(GPIO_NUM_4, GPIO_NUM_5);

void setup() {
  Serial.begin(115200);
  const char* label = "boiler ";

  controller.onRequest([label](const Message& request) {
    Serial.print(label);
    Serial.print("TX start: ");
    Serial.println(request.getRaw(), HEX);
  });
  controller.onResponse([label](const Message& response) {
    Serial.print(label);
    Serial.print("RX: ");
    Serial.println(response.getRaw(), HEX);
  });
  controller.onStatusChange([label](const Controller::StatusEvent& event) {
    if (event.message.isComplete() && !event.message.hasFrame()) {
      Serial.print(label);
      Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
    }
  });
  controller.begin();
}

void loop() {
  if (controller.isReady()) {
    controller.sendRequestAsync(ESP32OT::Messages::Status());
  }

  delay(750);
}
```

В роли master `onRequest` вызывается перед каждой попыткой локальной передачи
с `TRANSMITTING` и `hasFrame() == false`; последующая отмена или ошибка может
предотвратить передачу. Это не подтверждение завершения TX.
`onResponse` получает полные RX-кадры, включая невалидные. В роли slave `onRequest`
получает полные RX-запросы, а `onResponse` наблюдает исходящие ответы перед TX.
Оба hook регистрируются независимо в обеих ролях. Только RX-уведомления
гарантируют `hasFrame()` и не нуждаются в повторной проверке кадра. Таймаут
без полного кадра сообщается status-событием.

Прежняя регистрация функции с context доступна через `LegacyMessageCallback`
и `LegacyStatusCallback`. Например:

```cpp
static void responseReceived(const Message& response, void* context) {
  Serial.print(static_cast<const char*>(context));
  Serial.println(response.getRaw(), HEX);
}

static char responseLabel[] = "boiler RX ";
// In setup(), as an alternative to the response lambda:
// controller.onResponse(responseReceived, responseLabel);
```

Aliases `MessageCallback` и `StatusCallback` теперь обозначают одноаргументные
владеющие функции; для явно типизированных старых указателей используйте aliases
`Legacy`. Пустой callback или вызов setter без аргументов снимает этот hook.

Ссылки на сообщения callback временные: для дальнейшего использования сохраните
копии. Обработчики исполняются worker вне ISR/внутренних locks. Они должны быть
короткими и неблокирующими; TX-observer задерживает начало передачи. Синхронные
send/begin/end того же экземпляра отклоняются в worker. Пересылайте между
экземплярами асинхронно и избегайте циклического ожидания worker друг друга.

Непустая регистрация выделяет совместно владеемое хранилище callable; захваты
могут выделять дополнительную память. Вызов копирует совместное владение,
а не объект функции. Владение замыканием не распространяется на объекты по
захваченным ссылкам, `this` или legacy context. Замена/снятие hook не ждёт
работающий/подготовленный вызов. Сохраняйте заимствованное хранилище до завершения
таких вызовов; успешный `end()` завершает worker перед его разрушением.
Выбирайте роль до регистрации message-hooks: смена остановленной роли очищает
оба request/response hook, сохраняя status hook.

`DelayCallback` остаётся указателем функции с заимствованным context. Меняйте
или снимайте его только при полной остановке. Он обязан уступать CPU и не должен
повторно входить в синхронную операцию того же экземпляра.

## Завершение, повторы и время

`SUCCESS` — подтверждённые данные или успешная передача slave. `DATA_INVALID`
и `UNKNOWN_DATA_ID` — корректные отрицательные ответы: операция завершена,
`isSuccess()` ложен, payload не является подтверждённым измерением. `INVALID` —
повреждённый timing, ошибка parity/type либо ID ответа, не совпадающий с активным
запросом master. Passive RX не сравнивается с ID старого запроса.

`Config::maxAttempts` — полное число синхронных попыток master, включая первую;
по умолчанию 5, ноль означает одну. Успех, корректный отрицательный ответ, отмена
и остановка прекращают повторы. Невалидные ответы, таймауты и восстанавливаемые
ошибки повторяются при наличии попыток. Async и slave TX имеют одну попытку.
Callbacks сохраняют результат каждой попытки: успешный итог не отменяет
предыдущих ошибок обмена. Счётчик поддерживает 255 попыток без переполнения.

После успешной настройки аппаратуры `begin()` выдерживает TX в idle
(уровень 1) в течение 1150 мс перед переходом в READY.

TX: полуинтервалы 500 мкс, start, 32 бита MSB-first, stop. RX допускает отклонения
времени и асимметричные половины; восстановление фазы должно дать единственное
толкование по timing до проверки parity/type/ID. Чётность raw не исправляется.
Бюджет неактивности RX — 1 с плюс 40 мс frame grace и 2,5 мс capture idle. Это deadline,
а не измеренная задержка ответа. TX watchdog — 1 с. Драйвер выдерживает master 100 мс /
slave 20 мс от RX_DONE с округлением тиков вверх. Частоту опроса и политику
подключения/повторной инициализации определяет приложение. Для непрерывного
управления OpenTherm §4.3.1 требует обмена не реже одного раза в 1 с
(допуск +15%, MCI 1,15 с); приложение планирует запросы с учётом этого ограничения.
Длинные диагностические паузы и явные циклы end/begin не поддерживают непрерывный
сеанс управления. Serial из worker
занимает его время; для шлюза с ограничением задержки смотрите
[TransparentProxy](../examples/TransparentProxy/TransparentProxy.ino).

## Значения, raw и маршрутизация

`Message` содержит raw frame и metadata операции. Wrappers разбирают тот же
16-битный payload по запросу, без второго кеша и динамического выделения памяти.
`as<T>()` явно копирует значение в выбранный wrapper без RTTI и проверки ID.
Выбирайте wrapper, соответствующий реальному ID/формату.

`build()` и raw send сохраняют исходные биты, включая неверную чётность и vendor
fields. Именованные setters пересобирают изменённое поле/header/parity, сохраняя
посторонние payload bits и маршрутизацию принятого снимка. Для точной пересылки
используйте raw. `reply()` сохраняет ID и sequence запроса, создаёт новый ответ
и свежую metadata операции. Устаревший sequence не может ответить на новый запрос
или пережить end()/begin() как текущий ответ.

Числовые форматы представлены Float/UInt16/Int16/UInt8; структурированные ID —
именованными wrappers и флагами. ID30 сохраняет существующее толкование f8.8
при неоднозначности s16/f8.8 в спецификации. ID100 использует LB priority flags
подробной таблицы. Противоречивое положение system-type-bit ID103 не угадывается.
Единицы и доступные поля указаны у каждого wrapper и в полной таблице MessageID.

## Выбор примеров

| Задача | Пример / методы |
| --- | --- |
| Последовательный master | [Master](../examples/Master/Master.ino), `sendRequest` в `loop()` |
| Sync с callbacks каждой попытки | [MasterCallbacks](../examples/MasterCallbacks/MasterCallbacks.ino) |
| Неблокирующий master | [MasterAsync](../examples/MasterAsync/MasterAsync.ino), `isReady` и async sends |
| Все readable ID и TSP/FHB по индексам | [ReadAll](../examples/ReadAll/README.ru.md) |
| Проверка ID, необязательные настроенные записи, карта наблюдаемого доступа и счётчики статусов | [TestAllIDs](../examples/TestAllIDs/README.ru.md) |
| Модель slave и явные неподдерживаемые ответы | [Slave](../examples/Slave/Slave.ino), `onRequest` и async response |
| Две независимые OpenTherm-шины | [TransparentProxy](../examples/TransparentProxy/TransparentProxy.ino) |
| Heap, минимум крупнейшего блока, lifecycle и стек | [MasterHeap](../examples/MasterHeap/README.ru.md) |

Сравнивайте heap в совместимых allocation capability pools. Измеряйте свободную
память и крупнейший выделяемый блок, их наблюдавшиеся минимумы и максимум
`100*(1-largest/free)` по согласованным данным одного снимка; явно обработайте
free=0. Отношение показывает концентрацию свободной памяти, но не доказывает
полную исправность allocator. Heap, static RAM, IRAM, стеки задач и flash — разные
бюджеты. Периодические замеры могут пропустить более короткие провалы.
## Публичный facade: `ESP32OT::ESP32OT`

Исходники: [ESP32OT.h](../src/ESP32OT.h), [ESP32OT.cpp](../src/ESP32OT.cpp).
Некопируемый контроллер владеет одним независимым закреплённым FreeRTOS worker
и одним интерфейсом OpenTherm на RMT. Приложение использует facade для опроса
master, ответов slave, пассивного наблюдения и перенаправления между двумя
экземплярами. Внешний цикл `process()` не нужен. Namespace и класс намеренно
имеют одинаковое имя; используйте полные имена либо следующие локальные aliases.

```cpp
#include <ESP32OT.h>
using Controller = ESP32OT::ESP32OT;
using Message = ESP32OT::Messages::Message;
using Status = ESP32OT::Messages::Status;
```

Независимые фрагменты ниже предполагают настроенный master `boiler` и slave
`slave`. Для отправки нужен успешный `begin()`. `received` — копия текущего
принятого запроса slave, а не произвольный новый Message; `rawReply` и
`currentSequence` — подготовленный сырой ответ и sequence этого запроса.
Определения именованных callback-функций размещаются на уровне файла; лямбды
регистрируются на месте вызова. Намеренно отклоняемые
примеры показывают удалённые операции; внутренние примеры описывают контекст
реализации и не предназначены для вызова из приложения. GPIO должны подходить плате.

У facade нет публичных полей данных. Все поля конфигурации, StatusEvent, значения
enum и методы payload-wrapper описаны в разделах соответствующих типов.

### Aliases типов facade

Aliases не создают дополнительных данных либо преобразований. Ниже перечислены
все одиннадцать aliases, объявленных внутри facade.

#### `MessageID`

```cpp
using MessageID = ::ESP32OT::MessageID;
```

Enum идентификатора данных; выбирает ID payload.

```cpp
Controller::MessageID id = ESP32OT::MessageID::Tboiler;
```

#### `MessageType`

```cpp
using MessageType = ::ESP32OT::MessageType;
```

Enum типа/операции кадра; выбирает READ_DATA, WRITE_DATA либо тип ответа.

```cpp
Controller::MessageType type = ESP32OT::MessageType::READ_ACK;
```

#### `OperationStatus`

```cpp
using OperationStatus = ::ESP32OT::OperationStatus;
```

Локальное состояние операции, отдельное от MessageType на проводе.

```cpp
Controller::OperationStatus state = boiler.getResult().getStatus();
```

#### `Message`

```cpp
using Message = ::ESP32OT::Messages::Message;
```

Сырой кадр и метаданные операции по значению; база всех payload-wrapper.

```cpp
Controller::Message read = Message::read(ESP32OT::MessageID::Tboiler);
```

#### `Config`

```cpp
using Config = ::ESP32OT::Transport::Config;
```

Полная конфигурация GPIO, роли, повторов и worker; заменяется при остановке.

```cpp
Controller::Config cfg = boiler.getConfig();
```

#### `StatusEvent`

```cpp
using StatusEvent = ::ESP32OT::Transport::StatusEvent;
```

Представление callback с предыдущим/текущим статусом и временным const Message&.

```cpp
// Callback argument:
const Controller::StatusEvent& event = callbackEvent;
```

#### `StatusCallback`

```cpp
using StatusCallback = ::ESP32OT::Transport::StatusCallback;
```

std::function<void(const StatusEvent&)>; владеющий одноаргументный status-handler worker, включая захватывающие лямбды.

```cpp
Controller::StatusCallback hook = [](const Controller::StatusEvent& event) {
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
};
```

#### `MessageCallback`

```cpp
using MessageCallback = ::ESP32OT::Transport::MessageCallback;
```

std::function<void(const Messages::Message&)>; владеющий одноаргументный observer запросов/ответов для RX и pre-TX в обеих ролях.

```cpp
Controller::MessageCallback hook = [](const Message& message) {
  Serial.println(message.getRaw(), HEX);
};
```

#### `LegacyStatusCallback`

```cpp
using LegacyStatusCallback = ::ESP32OT::Transport::LegacyStatusCallback;
```

void (*)(const StatusEvent&, void*); указатель совместимости с явно заимствованным context.

```cpp
Controller::LegacyStatusCallback hook = statusChanged;
```

#### `LegacyMessageCallback`

```cpp
using LegacyMessageCallback = ::ESP32OT::Transport::LegacyMessageCallback;
```

void (*)(const Messages::Message&, void*); указатель request/response-функции совместимости с заимствованным context.

```cpp
Controller::LegacyMessageCallback hook = responseReceived;
```

#### `DelayCallback`

```cpp
using DelayCallback = void (*)(unsigned int milliseconds, void* context);
```

Hook ожидания/уступания CPU синхронным вызывающим; milliseconds задаёт запрос ожидания, context принадлежит вызывающему.

```cpp
Controller::DelayCallback hook = yieldCaller;
```

### Методы и специальные функции класса

#### `ESP32OT`

```cpp
ESP32OT();
```

Создаёт master с Config по умолчанию; применяется, если GPIO будут заданы позже.

Делегирует конструктору с Config. Worker и каналы RMT не запускаются. GPIO_NUM_NC по умолчанию нужно заменить через setConfig либо begin(rxGpio, txGpio).

```cpp
Controller boiler;
```

#### `explicit ESP32OT`

```cpp
explicit ESP32OT(const Config& config);
```

Создаёт контроллер из полной конфигурации приложения: master, slave или один из интерфейсов прокси.

Копирует конфигурацию и создаёт transport с указанной ролью. Исходный Config после этого не обязан существовать. Проверка аппаратных параметров и создание ресурсов происходят в begin(), а не в конструкторе.

```cpp
Controller::Config cfg;
cfg.rxGpio = GPIO_NUM_35;
cfg.txGpio = GPIO_NUM_36;
cfg.maxAttempts = 1;
Controller boiler(cfg);
```

#### `explicit ESP32OT`

```cpp
explicit ESP32OT(gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave = false, bool alwaysReceive = false);
```

Создаёт контроллер с GPIO платы и необязательным выбором slave/пассивного приёма.

Создаёт Config{rxGpio, txGpio, isSlave, alwaysReceive}; остальные поля сохраняют значения по умолчанию. RX — вход от адаптера в ESP32, TX — выход ESP32. Конструктор не запускает обмен.

```cpp
Controller boiler(GPIO_NUM_35, GPIO_NUM_36);
Controller thermostatSide(GPIO_NUM_4, GPIO_NUM_5, true, false);
```

#### `~ESP32OT`

```cpp
~ESP32OT();
```

Останавливает transport до уничтожения памяти контроллера; применяется и для локальных экземпляров.

Вызывает transport.end(), затем снимает все три владеющих observer-hook, пока конфигурация facade ещё существует. Захваченные объекты освобождаются до уничтожения полей facade. Ошибка очистки приводит к ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE), поскольку нельзя молча освободить память живого worker. Если ошибку остановки нужно обработать, вызывайте end() явно. Не уничтожайте экземпляр из его собственного callback worker.

```cpp
{
  Controller scoped(cfg);
  if (scoped.begin()) {
    auto result = scoped.sendRequest(Status());
    (void)result;
  }
} // Destructor stops the scoped transport.
```

#### `ESP32OT`

```cpp
ESP32OT(const ESP32OT&) = delete;
```

Запрещает копирование владельца worker, объектов синхронизации и ресурсов RMT.

Ограничение времени компиляции; действий во время исполнения нет. Передавайте Controller& или Controller* в функции. Следующий пример намеренно не компилируется.

```cpp
Controller copied(boiler); // Does not compile: copying is deleted.
```

#### `ESP32OT& operator=`

```cpp
ESP32OT& operator=(const ESP32OT&) = delete;
```

Запрещает замену владельца ресурсов копированием другого контроллера.

Запрет времени компиляции. Для смены настроек остановите экземпляр, скопируйте его Config и вызовите setConfig. Присваивание ниже не является обновлением конфигурации.

```cpp
other = boiler; // Does not compile: copy assignment is deleted.
```

#### `ESP32OT`

```cpp
ESP32OT(ESP32OT&&) = delete;
```

Сохраняет постоянный адрес контроллера для worker/ISR и владения callback.

Перемещение запрещено даже для остановленного экземпляра. Создавайте объект сразу в постоянном месте хранения либо передавайте ссылку. Пример намеренно отклоняется компилятором.

```cpp
Controller moved(static_cast<Controller&&>(boiler)); // Does not compile.
```

#### `ESP32OT& operator=`

```cpp
ESP32OT& operator=(ESP32OT&&) = delete;
```

Запрещает передачу владения ресурсами перемещающим присваиванием.

Ограничение времени компиляции без действий во время исполнения. Меняйте конфигурацию существующего остановленного экземпляра, а не присваивайте ему другой контроллер.

```cpp
other = static_cast<Controller&&>(boiler); // Does not compile.
```

#### `bool begin`

```cpp
bool begin();
```

Запускает worker и ресурсы RMT по сохранённой конфигурации; применяется при старте или после успешного end().

Ждёт подтверждения запуска worker, включая выдержку TX в idle (уровень 1) в течение 1150 мс после успешной настройки аппаратуры, и возвращает результат инициализации. Уже запущенный экземпляр и неверные аппаратные/задачные настройки отклоняются. false может сопровождаться ресурсами, требующими end(); это не доказательство полной очистки. Инициализация не подтверждает наличие отвечающего котла. Из ISR и callback собственного worker вызов запрещён.

```cpp
if (!boiler.begin()) {
  Serial.printf("begin error: %ld\n", static_cast<long>(boiler.getLastError()));
}
```

#### `bool begin`

```cpp
bool begin(gpio_num_t rxGpio, gpio_num_t txGpio);
```

Запускает экземпляр с новой парой GPIO, сохраняя остальные настройки.

Создаёт временную копию Config, заменяет RX/TX и запускает transport. Сохраняет новые GPIO в Config facade только при успешном запуске; при ошибке прежняя пара остаётся. Ограничения жизненного цикла и ошибки соответствуют begin().

```cpp
Controller boiler;
if (boiler.begin(GPIO_NUM_35, GPIO_NUM_36)) {
  auto savedRx = boiler.getConfig().rxGpio;
  (void)savedRx;
}
```

#### `bool end`

```cpp
bool end();
```

Совместно с worker останавливает callback, текущие/ожидающие операции и аппаратные ресурсы перед перенастройкой или уничтожением.

Запрашивает остановку worker и ждёт подтверждения очистки. Текущие/ожидающие операции завершаются CANCELLED. Для уже остановленного объекта возвращает true. false означает неполную остановку: её можно повторить, нельзя считать worker уничтоженным. В ISR и callback собственного worker вызов отклоняется. Успешная остановка завершает прежние callback, в отличие от простого снятия hook.

```cpp
if (boiler.end()) {
  auto cfg = boiler.getConfig();
  cfg.maxAttempts = 1;
  boiler.setConfig(cfg);
}
```

#### `bool isInitialized`

```cpp
bool isInitialized() const;
```

Проверяет, сообщает ли transport о готовой аппаратной инициализации без остановки.

Возвращает синхронизированный снимок состояния transport. Это не признак связи с устройством, успешного приёма или полной очистки после false/фатальной ошибки. Для подтверждённой остановки перед перенастройкой используйте end().

```cpp
if (!boiler.isInitialized()) {
  // Initialization/recovery policy belongs to the application.
}
```

#### `bool isReady`

```cpp
bool isReady() const;
```

Проверяет возможность следующей асинхронной операции при отсутствии текущей операции и протокольной выдержки.

Требует initialized, отсутствия stopping/busy и состояния READY драйвера. Учитывает межкадровую выдержку: операция может завершиться раньше готовности. Не подтверждает подключённый котёл. Другой вызывающий может изменить состояние после снимка, поэтому bool результата async-отправки всё равно нужно проверять.

```cpp
if (boiler.isReady()) {
  const bool accepted = boiler.sendRequestAsync(Status());
  (void)accepted;
}
```

#### `esp_err_t getLastError`

```cpp
esp_err_t getLastError() const;
```

Читает последнюю опубликованную ошибку transport/SDK для диагностики запуска, остановки и отправки.

Возвращает esp_err_t, а не OperationStatus. Обычный таймаут приёма либо валидный отрицательный ответ может иметь ESP_OK; статус Message проверяется отдельно. Последующая операция может заменить общую ошибку экземпляра, поэтому для конкретного обмена сохраняйте getError() его результата.

```cpp
esp_err_t error = boiler.getLastError();
Serial.printf("transport error: %ld\n", static_cast<long>(error));
```

#### `uint32_t getWorkerStackHighWaterMark`

```cpp
uint32_t getWorkerStackHighWaterMark() const;
```

Измеряет наименьший наблюдавшийся неиспользованный остаток стека worker для выбора его размера и бюджета callback.

Возвращает байты по данным данного порта ESP-IDF FreeRTOS либо ноль при отсутствии worker. Это стек, не heap; наблюдавшийся минимум по исполненным путям, а не гарантия худшего случая. Снимайте значение, пока worker существует, до успешного end().

```cpp
uint32_t unusedBytes = boiler.getWorkerStackHighWaterMark();
Serial.printf("worker unused stack: %lu B\n", static_cast<unsigned long>(unusedBytes));
```

#### `const Config& getConfig`

```cpp
const Config& getConfig() const;
```

Позволяет просмотреть сохранённые настройки либо получить копию для перенастройки остановленного экземпляра.

Возвращает принадлежащую facade const-ссылку, а не изменяемый дескриптор настроек или состояние удалённого устройства. Для правки полей сначала сделайте копию. Ссылка существует не дольше контроллера.

```cpp
auto cfg = boiler.getConfig();
cfg.maxAttempts = 3;
```

#### `bool setConfig`

```cpp
bool setConfig(const Config& config);
```

Полностью заменяет сохранённую конфигурацию остановленного экземпляра, включая выбор роли.

Захватывает session и меняет роль только при остановленном worker; при успехе копирует весь Config. Не объединяет лишь непустые поля. Смена роли очищает оба request/response hook, сохраняя status hook. Регистрируйте обработчики сообщений после выбора роли. Допустимость аппаратных и задачных аргументов проверяется в begin(). Возвращает false при недоступности session/перенастройки либо существующем worker.

```cpp
if (boiler.end()) {
  auto cfg = boiler.getConfig();
  cfg.maxAttempts = 3;
  const bool updated = boiler.setConfig(cfg);
  (void)updated;
}
```

#### `ESP32OT& setDelayCallback`

```cpp
ESP32OT& setDelayCallback(DelayCallback callback = nullptr, void* context = nullptr);
```

Настраивает уступание процессора синхронным вызывающим при ожидании, например для интеграции с планировщиком приложения.

Хранит только указатели функции/контекста и возвращает *this для цепочки вызовов. Настраивается или снимается только при полностью остановленном worker; изменение во время работы молча игнорируется. nullptr восстанавливает vTaskDelay минимум на один tick. Callback обязан уступать CPU и не должен повторно входить в синхронную операцию того же экземпляра: она может заменить снимок завершения. Этот hook не задаёт межкадровую политику протокола. Контекст обязан существовать во время всех вызовов.

```cpp
static void yieldCaller(unsigned int ms, void*) {
  const TickType_t ticks = pdMS_TO_TICKS(ms);
  vTaskDelay(ticks > 0 ? ticks : 1);
}
// Before begin():
boiler.setDelayCallback(yieldCaller, nullptr);
// After a successful end():
boiler.setDelayCallback();
```

#### `ESP32OT& onStatusChange`

```cpp
ESP32OT& onStatusChange(StatusCallback callback = nullptr);
```

Наблюдает переходы публичного статуса, включая ошибки без полного кадра.

Возвращает *this. Пустой callback снимает hook. Владеющий std::function принимает захватывающие лямбды. Worker вызывает его вне locks/ISR при фактической смене статуса. StatusEvent и его Message временные. Непустая регистрация выделяет совместно владеемое хранилище callable; вызов сохраняет владение без копирования callable. Захваченные ссылки/указатели остаются заимствованными. Обработчик должен быть коротким и неблокирующим; снятие/замена не ждёт завершения вызова. Успешный end() завершает вызовы worker перед разрушением захваченных объектов.

```cpp
const char* label = "boiler ";
boiler.onStatusChange([label](const Controller::StatusEvent& event) {
  Serial.print(label);
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
});
```

#### `ESP32OT& onStatusChange`

```cpp
ESP32OT& onStatusChange(LegacyStatusCallback callback, void* context = nullptr);
```

Сохраняет прежнюю регистрацию status-функции с context.

Адаптирует двухаргументную функцию и context во владеющий одноаргументный status-handler и возвращает *this. nullptr снимает hook. Context заимствован и должен существовать до завершения работающих/подготовленных вызовов. Остальные правила совпадают с основной перегрузкой.

```cpp
static void statusChanged(const Controller::StatusEvent& event, void*) {
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
}
boiler.onStatusChange(statusChanged, nullptr);
```

#### `bool onResponse`

```cpp
bool onResponse(MessageCallback callback = nullptr);
```

Наблюдает принятые ответы в роли master и исходящие ответы в роли slave.

Назначает/снимает независимый response-hook и возвращает true в обеих ролях. Master RX включает полные невалидные, отрицательные и незапрошенные пассивные кадры; hasFrame=true, но успех не гарантирован. Slave TX вызывает hook один раз перед каждой попыткой локальной передачи с TRANSMITTING и hasFrame=false. Последующая отмена или ошибка может предотвратить TX; hook не подтверждает завершение или доставку. Пустой callback снимает hook. std::function может захватывать состояние; правила выделения памяти, владения, временных ссылок и замены совпадают с onStatusChange. Долгий исходящий обработчик задерживает начало TX.

```cpp
const char* label = "boiler RX ";
const bool installed = boiler.onResponse([label](const Message& response) {
  Serial.print(label);
  Serial.println(response.getRaw(), HEX);
});
```

#### `bool onResponse`

```cpp
bool onResponse(LegacyMessageCallback callback, void* context = nullptr);
```

Сохраняет регистрацию response-функции с context в обеих ролях.

Адаптирует прежнюю двухаргументную функцию и заимствованный context в response-observer. Возвращает true, включая снятие hook через nullptr. Время вызова и RX/TX-метаданные совпадают с основной перегрузкой; изменение alias MessageCallback не требует менять эти вызовы регистрации. Для явно типизированного старого указателя используйте LegacyMessageCallback.

```cpp
static void responseReceived(const Message& response, void*) {
  Serial.println(response.getRaw(), HEX);
}
boiler.onResponse(responseReceived, nullptr);
```

#### `bool onRequest`

```cpp
bool onRequest(MessageCallback callback = nullptr);
```

Наблюдает исходящие запросы master либо полные принятые запросы slave.

Назначает/снимает независимый request-hook и возвращает true в обеих ролях. Master TX вызывает его один раз перед каждой попыткой локальной передачи с TRANSMITTING и hasFrame=false; последующая отмена или ошибка может предотвратить TX. Slave RX передаёт полные кадры, включая невалидные; обычный принятый запрос имеет REQUEST_RECEIVED. Невалидный шум по умолчанию не вытесняет сохранённый валидный запрос. Перед асинхронным ответом проверяйте статус/тип/ID. Синхронные send/begin/end того же экземпляра по-прежнему отклоняются в worker. Правила владения и замены совпадают с onStatusChange; пустой callback снимает hook.

```cpp
const char* label = "boiler TX ";
boiler.onRequest([label](const Message& request) {
  Serial.print(label);
  Serial.println(request.getRaw(), HEX);
});
```

#### `bool onRequest`

```cpp
bool onRequest(LegacyMessageCallback callback, void* context = nullptr);
```

Использует request-функцию с context, в том числе для асинхронного ответа slave.

Адаптер совместимости сохраняет функцию и заимствованный context, а не объект по указателю. Семантика обеих ролей совпадает с основной перегрузкой; возвращает true, nullptr снимает hook. Сохраняйте context до завершения работающих/подготовленных вызовов либо успешно завершите end() перед разрушением.

```cpp
static void requestReceived(const Message& request, void* context) {
  auto& slave = *static_cast<Controller*>(context);
  if (request.getStatus() != ESP32OT::OperationStatus::REQUEST_RECEIVED) {
    return;
  }

  if (request.getId() == ESP32OT::MessageID::Status) {
    if (request.getType() == ESP32OT::MessageType::READ_DATA) {
      slave.sendResponseAsync(request.as<Status>().reply(0));

    } else {
      slave.sendResponseAsync(request.reply(ESP32OT::MessageType::DATA_INVALID, 0));
    }

  } else {
    slave.sendResponseAsync(request.reply(ESP32OT::MessageType::UNKNOWN_DATA_ID, 0));
  }
}
slave.onRequest(requestReceived, &slave);
```

#### `bool sendRequestAsync`

```cpp
bool sendRequestAsync(const Message& request);
```

Передаёт один запрос master без блокировки вызывающего.

Возвращает факт принятия команды, а не успешного ответа котла. Нужны инициализированная роль master и отсутствие другой ожидающей/исполняемой операции. Worker обслуживает TX, RX и протокольные выдержки; внешний process() не нужен. Выполняется одна попытка независимо от Config::maxAttempts. Завершение доступно через getResult или callback; сохраняйте копии значений, не временные ссылки.

```cpp
if (boiler.isReady() && boiler.sendRequestAsync(Status())) {
  // Continue application work; observe completion later.
}
```

#### `bool sendResponseAsync`

```cpp
bool sendResponseAsync(const Message& response);
```

Передаёт ответ slave без блокировки worker, в особенности из onRequest.

Требует роли slave и ответа с текущим ненулевым sequence сохранённого запроса. request.reply сохраняет ID и маршрутизацию. Возвращает факт принятия; SUCCESS завершения означает окончание TX, поскольку master не подтверждает ответ slave. Одна попытка. Старый sequence либо другая текущая операция приводит к отказу. Перед TX worker выдерживает паузу ответа slave.

```cpp
const auto response = received.reply(ESP32OT::MessageType::UNKNOWN_DATA_ID, 0);
const bool accepted = slave.sendResponseAsync(response);
```

#### `Message sendRequest`

```cpp
Message sendRequest(const Message& request);
```

Синхронно отправляет общий запрос master и возвращает снимок итогового результата по значению.

Ожидает с уступанием CPU, пока работает независимый worker. Может дождаться ранее принятой async-операции. Master повторяет INVALID, TIMEOUT и восстановимые ошибки до maxAttempts суммарных попыток (ноль означает одну), останавливаясь при успехе, валидном отрицательном ответе, отмене или остановке. Возвращённый Message сохраняет метаданные запроса/операции; перед использованием данных проверяйте isSuccess, а перед трактовкой getRaw как принятого кадра — hasFrame. Вызов из ISR/собственного worker отклоняется как ERROR.

```cpp
auto result = boiler.sendRequest(Message::read(ESP32OT::MessageID::Tboiler));
if (result.isSuccess()) {
  const float value = result.as<ESP32OT::Messages::Float>().get();
  Serial.println(value);
}
```

#### `Message sendResponse`

```cpp
Message sendResponse(const Message& response);
```

Отправляет общий ответ slave из задачи приложения и ждёт результата передачи.

Использует одну попытку и проверяет текущий sequence/роль. SUCCESS подтверждает TX slave, а не последующий ACK от master. В callback собственного worker блокирующий sendResponse отклоняется; там используйте sendResponseAsync. Стройте ответ из всё ещё текущего принятого запроса: устаревший снимок маршрутизации будет отклонён.

```cpp
// Application task, after obtaining a current accepted request:
auto response = received.reply(ESP32OT::MessageType::UNKNOWN_DATA_ID, 0);
auto sent = slave.sendResponse(response);
```

#### `T sendRequest`

```cpp
template<class T, typename std::enable_if<std::is_base_of<Message, T>::value, int>::type = 0> T sendRequest(const T& request);
```

Отправляет типизированный запрос master и сохраняет тот же тип wrapper у результата.

Доступен только для T, производного от Message. Копирует переданный T и разбирает итоговый снимок метаданных в этой копии. Не выбирает wrapper по принятому ID и не гарантирует успешность полного payload. Ограничения роли, ожидания и повторов совпадают с общим overload; единицы и смысл getter определяются T.

```cpp
auto result = boiler.sendRequest(ESP32OT::Messages::Float(ESP32OT::MessageID::Tboiler));
if (result.isSuccess()) {
  Serial.printf("Flow: %.2f C\n", result.get());
}
```

#### `T sendResponse`

```cpp
template<class T, typename std::enable_if<std::is_base_of<Message, T>::value, int>::type = 0> T sendResponse(const T& response);
```

Передаёт типизированный ответ slave, сохраняя тип wrapper для анализа результата TX.

Копирует T, вызывает тот же синхронный механизм ответа и разбирает результат в копии. Требования sequence, одной попытки и запрет синхронного вызова из callback совпадают с общим методом ответа; одного создания wrapper недостаточно для маршрутизации запроса.

```cpp
// Application task; received is a current ID25 read request.
auto typed = received.as<ESP32OT::Messages::Float>();
auto response = typed.reply(ESP32OT::MessageType::READ_ACK, 35.5f);
auto sent = slave.sendResponse(response);
```

#### `Message getResult`

```cpp
Message getResult() const;
```

Копирует последний публичный результат transport либо промежуточное состояние для async-опроса и диагностики.

Возвращает синхронизированный снимок по значению без ожидания. Возможны QUEUED/TRANSMITTING/WAITING либо итоговый результат; проверяйте isComplete и при необходимости getOperation. Готовность шины и связь с устройством — отдельные понятия. Возможны независимые события пассивного RX; следующая операция может заменить последний результат экземпляра.

```cpp
auto result = boiler.getResult();
if (result.isComplete()) {
  Serial.println(ESP32OT::Protocol::Frame::statusToString(result.getStatus()));
}
```

#### `Message getRequest`

```cpp
Message getRequest() const;
```

Копирует последний принятый запрос со стороны slave для отложенного ответа или проверки маршрутизации.

Возвращает сохранённый запрос с ненулевым sequence для принятого запроса slave. У master изначально пустой снимок; это не исходящий запрос master (он доступен как result.getRequest()). end/begin очищает сохранённый запрос; новый запрос заменяет прежний.

```cpp
auto request = slave.getRequest();
if (request.getSequence() != 0) {
  auto id = request.getId();
  (void)id;
}
```

#### `void getResult`

```cpp
void getResult(Message& destination) const;
```

Обновляет существующий Message либо производный wrapper последним снимком результата.

Вызывает destination.parse с синхронизированным снимком transport, сохраняя C++-тип destination; автоматического выбора wrapper по ID нет. Не возвращает значение и не ждёт. Смысл payload выбирает вызывающий с учётом статуса/ID.

```cpp
ESP32OT::Messages::Float result;
boiler.getResult(result);
if (result.isSuccess() && result.getId() == ESP32OT::MessageID::Tboiler) {
  Serial.println(result.get());
}
```

#### `void getRequest`

```cpp
void getRequest(Message& destination) const;
```

Обновляет существующий wrapper последним сохранённым запросом slave без предварительного общего локального снимка.

Вызывает destination.parse для сохранённого запроса. Не проверяет соответствие ожидаемого формата destination его ID. До отправки ответа запрос может устареть; текущий sequence всё равно проверяется при передаче. Значение не возвращается, ожидания нет.

```cpp
Status request;
slave.getRequest(request);
if (request.getSequence() != 0 && request.getId() == ESP32OT::MessageID::Status) {
  const bool heating = request.isCentralHeatingEnabled();
  (void)heating;
}
```

#### `T getResult`

```cpp
template<class T> T getResult() const;
```

Возвращает последний результат сразу как выбранный вызывающим wrapper с конструктором по умолчанию.

Создаёт T по умолчанию и вызывает parse со снимком transport. T обязан поддерживать эти операции; в отличие от типизированной отправки у декларации нет явного enable_if. Не проверяет ID и не ждёт. Соответствующего getRequest<T>() нет: используйте getRequest().as<T>() либо overload с destination.

```cpp
auto result = boiler.getResult<ESP32OT::Messages::Float>();
if (result.isSuccess() && result.getId() == ESP32OT::MessageID::Tboiler) {
  Serial.println(result.get());
}
```

#### `Message sendRawRequest`

```cpp
Message sendRawRequest(uint32_t frame);
```

Синхронно передаёт полный сырой кадр master для vendor ID, неизвестных ID или прозрачного перенаправления.

Оборачивает все переданные 32 бита в Message и использует обычный синхронный механизм master с повторами. Не пересчитывает чётность и не очищает зарезервированные/vendor биты. Возвращает общий Message; все биты полного невалидного ответа сохраняются. Биты типа ответа не заменяют выбор роли slave.

```cpp
const uint32_t rawRequest = ESP32OT::Protocol::Frame::build(
  ESP32OT::MessageType::READ_DATA, ESP32OT::MessageID::Tboiler, 0
);
auto result = boiler.sendRawRequest(rawRequest);
```

#### `Message sendRawResponse`

```cpp
Message sendRawResponse(uint32_t frame, uint32_t requestSequence);
```

Синхронно передаёт точный сырой ответ slave, привязанный к текущему сохранённому запросу.

Создаёт MessageData с frame и requestSequence и вызывает sendResponse. Ненулевой sequence обязан соответствовать последнему принятому запросу и не может сохранять актуальность после end/begin. Чётность не исправляется, сырой ID не меняется. Одна попытка; SUCCESS означает завершённый TX. Используется вне callback этого worker, где нужен асинхронный вариант.

```cpp
// Application task; received is the current accepted request.
const auto reply = received.reply(ESP32OT::MessageType::UNKNOWN_DATA_ID, 0);
auto sent = slave.sendRawResponse(reply.build(), received.getSequence());
```

#### `bool sendRawRequestAsync`

```cpp
bool sendRawRequestAsync(uint32_t frame);
```

Передаёт точный сырой кадр master без ожидания, например в прозрачном прокси.

Сохраняет все 32 бита и использует однопопыточный sendRequestAsync. bool означает принятие команды, а не валидность ответа или связь. При перенаправлении между независимыми экземплярами сохраняйте контекст маршрутизации приложения. Тайминг обслуживает worker; нельзя блокировать один worker синхронным ожиданием другого.

```cpp
const bool accepted = boiler.sendRawRequestAsync(rawRequest);
```

#### `bool sendRawResponseAsync`

```cpp
bool sendRawResponseAsync(uint32_t frame, uint32_t requestSequence);
```

Асинхронно перенаправляет точный сырой ответ slave с соответствующим текущим sequence запроса.

Копирует сырой кадр и requestSequence в команду по значению, сохраняя даже неверную чётность при явно выбранной raw-передаче. Проверки роли, инициализации, единственной операции и актуальности sequence остаются. Возвращает принятие, не завершение; master не присылает ACK на этот ответ.

```cpp
const bool accepted = slave.sendRawResponseAsync(rawReply, currentSequence);
```

#### `::ESP32OT::Protocol::MessageData sendMessage`

```cpp
::ESP32OT::Protocol::MessageData sendMessage(const Message& message, bool response);
```

**Private: деталь реализации.** Внутренняя синхронная координация общих и типизированных запросов/ответов.

Private: приложение не может вызывать этот метод. response=false выбирает запрос master, true — ответ slave. Удерживает session на всех попытках, уступает CPU через wait, читает отдельный снимок синхронного завершения, ограничивает общее число попыток и возвращает итоговые метаданные с суммарными микросекундами. Отличается от Transport::RmtTransport::sendMessage: тот принимает одну команду для worker и немедленно возвращает bool. Вызовы из ISR/собственного worker отклоняются до ожидания.

```cpp
// Implementation context only; not an application-callable API:
result.parse(sendMessage(request, false));
```

#### `void wait`

```cpp
void wait(unsigned int milliseconds);
```

**Private: деталь реализации.** Внутреннее уступание CPU синхронным вызывающим; используется во время выполнения прежней операции либо текущей попытки.

Private: вызывает установленный DelayCallback(milliseconds, delayContext) либо vTaskDelay с pdMS_TO_TICKS минимум на один tick. Не обслуживает разбор RX и не задаёт протокольные deadline. Worker работает независимо. Пользовательский hook обязан уступать CPU и не может рекурсивно выполнять синхронную отправку на том же facade.

```cpp
// Implementation context only:
while (transport.isBusy() && isInitialized()) {
  wait(10);
}
```

### Приватные поля

Поля перечислены для сопровождающих библиотеку; приложение использует публичные методы.

#### `::ESP32OT::Transport::RmtTransport transport`

Владеет worker/driver/синхронизацией, принимает команды и предоставляет снимки. Создаётся с Config::isSlave.

```cpp
// Class implementation only:
return transport.isInitialized();
```

#### `Config config`

Сохранённая полная конфигурация, копируемая конструкторами/setConfig. Публичный доступ через const getConfig().

```cpp
// Application equivalent:
auto copy = boiler.getConfig();
```

#### `DelayCallback delayCallback = nullptr`

Необязательная невладеющая функция ожидания вызывающего, меняется только при полной остановке.

```cpp
// Public setter:
boiler.setDelayCallback(yieldCaller);
```

#### `void* delayContext = nullptr`

Невладеющий контекст delayCallback; facade не выделяет и не освобождает его.

```cpp
// Public setter; waitContext must outlive callback use:
boiler.setDelayCallback(yieldCaller, &waitContext);
```
## Модель сообщений и полный справочник wrappers

Все классы этого раздела находятся в ESP32OT::Messages. Message — конкретная полиморфная база; остальные 30 классов final и наследуют Message. Они хранят один Protocol::MessageData, без второго payload cache и без динамического выбора wrapper.

Конструкторы из snapshot/Message и as<T>() сохраняют исходные raw bits и всю metadata без проверки ID. build() возвращает raw без исправления parity. Именованные setters пересчитывают parity и сохраняют посторонние payload bits, но это не raw-пересылка reserved header bits. Для свежего NONE/sequence0 сообщения request следует изменениям; для принятого снимка исходный request сохраняется.

reply() сохраняет source ID и sequence, записывает source frame в request; operation/status/attempt/timing создаются заново. Для ответа на запрос используйте reply(), а не отдельный конструктор с теми же данными. Создание/изменение wrapper не выполняет I/O, не проверяет поддержку ID и не включает отопление само по себе.

В таблицах приведена каждая явно объявленная перегрузка, включая конструкторы и внутренние методы. Все 30 наследников также получают публичные методы Message; специализированные read/write/reply могут скрывать одноимённые фабрики базы. Доступ к базовой перегрузке при необходимости явно квалифицируется. Автоматическое копирование объектов сохраняет значение снимка.

Примеры вызовов предполагают следующие aliases и уже полученный result. m в таблице означает объект текущего класса, обычно result.as<M::Class>(); snapshot используется для конструкторов/parse. Private/protected вызовы показывают контекст внутри реализации, а не доступный приложению API. WRITE-примеры создают объекты и требуют подходящего writable ID перед реальной отправкой; ID200 в примере Int16 — условный vendor ID, не стандартный параметр.

```cpp
namespace M = ESP32OT::Messages;
using ID = ESP32OT::MessageID;
using MT = ESP32OT::MessageType;
// result is an ESP32OT::Messages::Message returned by a real operation.
const auto snapshot = result.getMessageData();
```

### Messages::Message

Конкретный базовый класс raw-кадра и снимка операции. Применяется для произвольных ID/полей производителя, логирования, маршрутизации ответа и явного выбора формата payload.

Исходник: [Message.h](../src/Messages/Message.h).

Protected поле data: Protocol::MessageData; единственное хранилище raw-кадра и metadata. Используется самой базой и наследниками, не является приложением управляемым вторым payload.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `explicit Message(uint32_t frame = 0)` (public) | Сохраняет frame и request=frame; status=NONE, hasFrame=false, прочие поля по умолчанию. Для raw-запроса/снимка; не подтверждает получение кадра. | `M::Message m{result.getRaw()};` |
| `explicit Message(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Message m{snapshot};` |
| `virtual ~Message()` (public) | Виртуальный default-деструктор базы: разрушение через Message* корректно; собственных динамических payload/ресурсов не освобождает. Обычно вызывается автоматически. | `{ M::Message local; } // scope exit` |
| `virtual uint32_t build() const` (public) | Возвращает сохранённый 32-битный кадр без изменений, включая reserved/vendor bits и неверную чётность; не отправляет, не проверяет и не исправляет parity. Для raw-пересылки и логирования. | `auto value = m.build();` |
| `virtual void parse(const Protocol::MessageData& snapshot)` (public) | Заменяет снимок целиком переданным MessageData, без изменения raw-битов и проверки ID. Для обновления переиспользуемого объекта результатом транспорта. | `m.parse(snapshot);` |
| `template<class T> T as() const` (public) | Возвращает значение выбранного вызывающим типа T через конструирование T из текущего сообщения; T обязан наследовать Message. Копирует raw и metadata, без проверки ID/формата, выделения heap и RTTI. Выбор корректного wrapper лежит на приложении. | `auto typed = m.as<M::Float>();` |
| `const Protocol::MessageData& getMessageData() const` (public) | Возвращает const-ссылку на полный принадлежащий объекту снимок; живёт не дольше объекта и отражает изменения parse/setters. Для хранения после callback копируйте значение. | `auto value = m.getMessageData();` |
| `uint32_t getRaw() const` (public) | Возвращает сохранённый raw-кадр без изменения. Перед трактовкой как полного принятого кадра проверяйте hasFrame. При успешной TX slave здесь остаётся отправленный ответ, даже если hasFrame=false; созданный вручную запрос также содержит raw независимо от этого флага. Для raw-логирования/пересылки с учётом направления. | `auto value = m.getRaw();` |
| `uint32_t getRequest() const` (public) | Возвращает сохранённый кадр запроса. Результат master содержит исходящий запрос; reply builder — кадр запроса, на который отвечает. Для сопоставления обменов и индексов текстовых запросов. | `auto value = m.getRequest();` |
| `uint16_t getData() const` (public) | Возвращает payload bits15..0 без проверки валидности и формата. Для диагностики raw-полей или собственного wrapper известного формата. | `auto value = m.getData();` |
| `uint8_t getHighByte() const` (public) | Возвращает биты15..8 payload, 0..255. Значение зависит от ID; это не обязательно число или байт флагов. | `auto value = m.getHighByte();` |
| `uint8_t getLowByte() const` (public) | Возвращает биты7..0 payload, 0..255. Трактовка зависит от ID. | `auto value = m.getLowByte();` |
| `Protocol::MessageID getId() const` (public) | Извлекает MessageID из битов23..16 без проверки поддержки ID или соответствия wrapper. Для выбора явного as<T>(). | `auto value = m.getId();` |
| `Protocol::MessageType getType() const` (public) | Извлекает 3-битный MessageType. Для различения запросов чтения/записи, ACK и корректных отрицательных ответов. | `auto value = m.getType();` |
| `Protocol::OperationStatus getStatus() const` (public) | Возвращает статус операции, а не только wire-тип сообщения. Вместе с hasFrame/isSuccess/isComplete различает данные, отрицательные ответы и сбои транспорта. | `auto value = m.getStatus();` |
| `uint32_t getOperation() const` (public) | Возвращает счётчик операций транспорта для сопоставления принятой исходящей работы и её результата; это не идентификатор котла или соединения. | `auto value = m.getOperation();` |
| `uint32_t getSequence() const` (public) | Возвращает sequence маршрутизации входящего запроса, сохраняемый в ответе. Для отсечения устаревших ответов slave/прокси; не заменяйте его при ответе на принятый запрос. | `auto value = m.getSequence();` |
| `uint32_t getElapsedUs() const` (public) | Возвращает длительность попытки в мкс от принятия транспортом, включая очередь/минимальное ожидание. Для самопроизвольного приёма равна нулю. Это не чистая задержка ответа котла. | `auto value = m.getElapsedUs();` |
| `uint32_t getTotalElapsedUs() const` (public) | В возвращённом конечном результате синхронного facade-вызова — общая длительность в мкс со всеми попытками/ожиданиями. В per-attempt callback/async снимках — длительность попытки; весь вызов профилируйте по возвращённому результату. | `auto value = m.getTotalElapsedUs();` |
| `uint8_t getAttempt() const` (public) | Возвращает номер исходящей попытки, начиная с1; синхронный master увеличивает его при повторах, async/slave TX используют1, свежие/самопроизвольные снимки0. Для различения первого ответа и результата повтора. | `auto value = m.getAttempt();` |
| `esp_err_t getError() const` (public) | Возвращает ESP-IDF esp_err_t снимка. ESP_OK сам по себе не означает успешные данные; проверяйте getStatus(). Для диагностики ошибок ресурсов/API. | `auto value = m.getError();` |
| `bool hasFrame() const` (public) | Показывает наличие полного принятого кадра; true возможен при INVALID и корректных отрицательных статусах. Таймаут/отмена могут завершиться без принятого кадра. Результат TX slave может хранить исходящий frame при false. Флаг нужен для разбора/логирования принятого кадра, а не исходящего. | `auto value = m.hasFrame();` |
| `bool isSuccess() const` (public) | True только при OperationStatus::SUCCESS: подтверждённые данные либо завершённая TX slave. False при DATA_INVALID/UNKNOWN_DATA_ID, хотя это корректные ответы. Для допуска к успешным данным. | `auto value = m.isSuccess();` |
| `bool isComplete() const` (public) | True для SUCCESS, INVALID, TIMEOUT, ERROR, CANCELLED, DATA_INVALID, UNKNOWN_DATA_ID. False для NONE/QUEUED/TRANSMITTING/WAITING/REQUEST_RECEIVED. Проверяет завершённость, а не валидность данных. | `auto value = m.isComplete();` |
| `static Message read(Protocol::MessageID id, uint16_t payload = 0)` (public) | Создаёт READ_DATA с id и полным 16-битным payload (по умолчанию0), новой NONE metadata и request=новый frame. Ненулевой payload нужен для индексных/vendor-запросов. | `auto value = M::Message::read(ID::TSPindexTSPvalue, 0x0300);` |
| `static Message write(Protocol::MessageID id, uint16_t payload)` (public) | Создаёт WRITE_DATA с id и полным 16-битным payload и новой NONE metadata. Поддержку, разрешение и смысл записи определяет приложение. | `auto value = M::Message::write(ID::TSet, 0x3700);` |
| `Message reply(Protocol::MessageType type, uint16_t payload) const` (public) | Создаёт ответ выбранного типа/payload с ID исходного сообщения; сохраняет sequence, request=исходный frame. Прочая operation/status/timing metadata новая. Для callback запросов slave/прокси. | `auto value = m.reply(MT::READ_ACK, 0x3700);` |
| `void setData(uint16_t payload)` (protected) | Protected helper реализации. Пересобирает frame с текущими type/ID, заданным payload и рассчитанной parity. Сохраняет metadata принятой операции и исходный request; request обновляется только для нового NONE/sequence0 сообщения. | `Внутри реализации: setData(0x3700);` |
| `void setFlag(uint16_t mask, bool value)` (protected) | Protected helper реализации. Устанавливает/сбрасывает заданную 16-битную маску payload через setData, сохраняя остальные биты. Применяется именованными bool-setters наследников. | `Внутри реализации: setFlag(0x0100, true);` |
| `static Protocol::MessageData createData(Protocol::MessageType type, Protocol::MessageID id, uint16_t payload)` (protected) | Protected static helper реализации. Создаёт frame и исходный request из type/id/payload; прочие поля MessageData сохраняют defaults. Для конструкторов/фабрик wrapper. | `Внутри реализации: createData(MT::READ_ACK, ID::Tboiler, 0x3700);` |
| `Protocol::MessageData createReply(Protocol::MessageType type, uint16_t payload) const` (protected) | Protected helper реализации. Создаёт новый MessageData текущего ID с заданным ответом, копирует sequence, request=исходный frame. Для всех типизированных reply builders с сохранением маршрута slave. | `Внутри реализации: createReply(MT::READ_ACK, 0x3700);` |

### Messages::BrandCharacter

ID93/94/95: чтение выбирает индекс символа в HB; в ответе HB содержит максимальный индекс, LB — символ. Индекс запроса берётся из сохранённой metadata, а не HB ответа. Для посимвольного чтения бренда/версии/серийного номера.

Исходник: [BrandCharacter.h](../src/Messages/BrandCharacter.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `BrandCharacter()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::BrandCharacter m{};` |
| `explicit BrandCharacter(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::BrandCharacter m{snapshot};` |
| `explicit BrandCharacter(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::BrandCharacter m{result};` |
| `explicit BrandCharacter(Protocol::MessageID id, uint8_t index = 0)` (public) | Создаёт READ_DATA: индекс в HB, LB=0, новая metadata. Для посимвольного/индексного чтения; id выбирает поддерживаемую группу. | `M::BrandCharacter m{ID::Brand, 0};` |
| `uint8_t getRequestIndex() const` (public) | Возвращает индекс символа из HB сохранённого исходного запроса; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getRequestIndex();` |
| `uint8_t getMaxIndex() const` (public) | Возвращает максимальный индекс символа из HB ответа; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMaxIndex();` |
| `char getCharacter() const` (public) | Возвращает символ из LB ответа; строка не выделяется; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getCharacter();` |
| `BrandCharacter reply(Protocol::MessageType type, uint8_t maxIndex, char character) const` (public) | Тип явно передан. В ответе HB=maxIndex, LB=character; выбранный индекс остаётся в request metadata. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 4, 'A');` |

### Messages::Capacity

ID15: HB — максимальная мощность котла в кВт, LB — минимальная модуляция в процентах. Для ограничений интерфейса и диапазона регулирования; конвертации единиц и проверки 0..100 нет.

Исходник: [Capacity.h](../src/Messages/Capacity.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Capacity()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::Capacity m{};` |
| `explicit Capacity(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Capacity m{snapshot};` |
| `explicit Capacity(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Capacity m{result};` |
| `Capacity( uint8_t maxCapacity, uint8_t minModulation, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей maxCapacity, minModulation, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Capacity m{24, 20, MT::READ_ACK};` |
| `uint8_t getMaxCapacity() const` (public) | Возвращает максимальная мощность котла, HB, кВт; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMaxCapacity();` |
| `uint8_t getMinModulation() const` (public) | Возвращает минимальная модуляция котла, LB, проценты; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMinModulation();` |
| `Capacity reply(Protocol::MessageType type, uint8_t maxCapacity, uint8_t minModulation) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 24, 20);` |
| `static uint16_t encode(uint8_t maxCapacity, uint8_t minModulation)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(24, 20);` |

### Messages::Date

ID21: HB — месяц, LB — день месяца. Применяется вместе с DayTime и UInt16 Year; проверки календарной даты нет.

Исходник: [Date.h](../src/Messages/Date.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Date()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::Date m{};` |
| `explicit Date(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Date m{snapshot};` |
| `explicit Date(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Date m{result};` |
| `Date( uint8_t month, uint8_t day, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей month, day, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Date m{11, 1, MT::WRITE_DATA};` |
| `uint8_t getMonth() const` (public) | Возвращает месяц, HB; приложение должно передавать 1..12; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMonth();` |
| `uint8_t getDay() const` (public) | Возвращает день месяца, LB; приложение проверяет допустимость для месяца/года; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getDay();` |
| `Date reply(Protocol::MessageType type, uint8_t month, uint8_t day) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 11, 1);` |
| `static uint16_t encode(uint8_t month, uint8_t day)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(11, 1);` |

### Messages::DayTime

ID20: биты7..5 HB — день недели (1 понедельник..7 воскресенье, 0 неизвестен), биты4..0 HB — часы, LB — минуты. Для часов устройства. Кодирование ограничивает разрядность, но не проверяет календарные диапазоны.

Исходник: [DayTime.h](../src/Messages/DayTime.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `DayTime()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::DayTime m{};` |
| `explicit DayTime(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::DayTime m{snapshot};` |
| `explicit DayTime(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::DayTime m{result};` |
| `DayTime( uint8_t dayOfWeek, uint8_t hours, uint8_t minutes, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей dayOfWeek, hours, minutes, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::DayTime m{1, 12, 30, MT::WRITE_DATA};` |
| `uint8_t getDayOfWeek() const` (public) | Возвращает день недели, биты7..5 HB: 0 неизвестен, 1 понедельник..7 воскресенье; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getDayOfWeek();` |
| `uint8_t getHours() const` (public) | Возвращает часы, биты4..0 HB; приложение должно передавать 0..23; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getHours();` |
| `uint8_t getMinutes() const` (public) | Возвращает минуты, LB; приложение должно передавать 0..59; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMinutes();` |
| `DayTime reply(Protocol::MessageType type, uint8_t dayOfWeek, uint8_t hours, uint8_t minutes) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 1, 12, 30);` |
| `static uint16_t encode(uint8_t dayOfWeek, uint8_t hours, uint8_t minutes)` (private) | Внутренний private helper. Маскирует день до3 бит и часы до5; минуты сохраняют8 бит, календарные диапазоны не проверяются. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(1, 12, 30);` |

### Messages::FanSpeed

ID35: HB — заданная, LB — фактическая частота вентилятора, обе в Гц (об/мин / 60), беззнаковые байты. Для сравнения задания и результата; для об/мин приложение умножает на 60.

Исходник: [FanSpeed.h](../src/Messages/FanSpeed.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `FanSpeed()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::FanSpeed m{};` |
| `explicit FanSpeed(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::FanSpeed m{snapshot};` |
| `explicit FanSpeed(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::FanSpeed m{result};` |
| `FanSpeed( uint8_t setpointHz, uint8_t actualHz, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей setpointHz, actualHz, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::FanSpeed m{50, 48, MT::READ_ACK};` |
| `uint8_t getSetpointHz() const` (public) | Возвращает заданная частота вентилятора, HB, Гц (об/мин / 60); только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSetpointHz();` |
| `uint8_t getActualHz() const` (public) | Возвращает фактическая частота вентилятора, LB, Гц (об/мин / 60); только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getActualHz();` |
| `FanSpeed reply(Protocol::MessageType type, uint8_t setpointHz, uint8_t actualHz) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 50, 48);` |
| `static uint16_t encode(uint8_t setpointHz, uint8_t actualHz)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(50, 48);` |

### Messages::Fault

ID5: прикладные флаги неисправностей в HB, OEM-код в LB. Для отображения ошибок и определения разрешения удалённого сброса. Флаг reset-enabled — разрешение, а не признак произошедшей блокировки.

Исходник: [Fault.h](../src/Messages/Fault.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Fault()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::Fault m{};` |
| `explicit Fault(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Fault m{snapshot};` |
| `explicit Fault(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Fault m{result};` |
| `Fault( uint8_t flags, uint8_t oemCode, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей flags, oemCode, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Fault m{0, 0, MT::READ_ACK};` |
| `bool isServiceRequested() const` (public) | Возвращает требование обслуживания; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isServiceRequested();` |
| `bool isLockoutResetEnabled() const` (public) | Возвращает разрешение удалённого снятия блокировки; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.isLockoutResetEnabled();` |
| `bool isLowWaterPressureFault() const` (public) | Возвращает ошибка низкого давления воды; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.isLowWaterPressureFault();` |
| `bool isGasFlameFault() const` (public) | Возвращает ошибка газа/пламени; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0800. | `auto value = m.isGasFlameFault();` |
| `bool isAirPressureFault() const` (public) | Возвращает ошибка давления воздуха; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x1000. | `auto value = m.isAirPressureFault();` |
| `bool isWaterOvertemperatureFault() const` (public) | Возвращает ошибка перегрева воды; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x2000. | `auto value = m.isWaterOvertemperatureFault();` |
| `uint8_t getOemFaultCode() const` (public) | Возвращает код неисправности производителя, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getOemFaultCode();` |
| `Fault& setServiceRequested(bool value)` (public) | Изменяет требование обслуживания; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setServiceRequested(true);` |
| `Fault& setLockoutResetEnabled(bool value)` (public) | Изменяет разрешение удалённого снятия блокировки; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setLockoutResetEnabled(true);` |
| `Fault& setLowWaterPressureFault(bool value)` (public) | Изменяет ошибка низкого давления воды; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setLowWaterPressureFault(true);` |
| `Fault& setGasFlameFault(bool value)` (public) | Изменяет ошибка газа/пламени; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0800. | `m.setGasFlameFault(true);` |
| `Fault& setAirPressureFault(bool value)` (public) | Изменяет ошибка давления воздуха; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x1000. | `m.setAirPressureFault(true);` |
| `Fault& setWaterOvertemperatureFault(bool value)` (public) | Изменяет ошибка перегрева воды; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x2000. | `m.setWaterOvertemperatureFault(true);` |
| `Fault& setOemFaultCode(uint8_t oemCode)` (public) | Изменяет код неисправности производителя, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Fault& для цепочки; для редактирования локального сообщения/ответа. | `m.setOemFaultCode(0);` |
| `Fault reply(Protocol::MessageType type, uint8_t flags, uint8_t oemCode) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 0);` |
| `Fault reply(uint8_t flags, uint8_t oemCode) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 0);` |
| `static uint16_t encode(uint8_t flags, uint8_t oemCode)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 0);` |

### Messages::Float

Знаковый f8.8: -128..127,99609375, шаг 1/256. Для температур, давления, расхода, модуляции и версий в формате f8.8; единицы определяются ID. Для ID30 сохранена существующая трактовка f8.8, несмотря на противоречащие описания s16.

Исходник: [Float.h](../src/Messages/Float.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Float()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::Float m{};` |
| `explicit Float(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Float m{snapshot};` |
| `explicit Float(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Float m{result};` |
| `explicit Float(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::Float m{ID::Tboiler};` |
| `Float( Protocol::MessageID id, float value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Float m{ID::TSet, 45.5f, MT::WRITE_DATA};` |
| `float get() const` (public) | Возвращает знаковый payload /256; единицы зависят от id; без проверки ACK/ID. Для декодирования успешного результата. | `auto value = m.get();` |
| `static Float read(Protocol::MessageID id)` (public) | Фабрика READ_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::Float::read(ID::Tboiler);` |
| `static Float write(Protocol::MessageID id, float value)` (public) | Фабрика WRITE_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::Float::write(ID::TSet, 45.5f);` |
| `Float reply(Protocol::MessageType type, float value) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 45.5f);` |
| `static Float writeTemperature(Protocol::MessageID id, float temperature)` (public) | Создаёт WRITE_DATA f8.8 после ограничения температуры 0..100 °C; NaN затем кодируется0. Это legacy helper только для нуждающихся в таком ограничении ID, не универсальная проверка всех температур. | `auto value = M::Float::writeTemperature(ID::TSet, 55.0f);` |
| `static uint16_t encode(float value)` (public) | Кодирует float в signed f8.8: NaN/inf→0; ограничивает -128..32767/256 и отбрасывает дробные разряды после умножения на256. Возвращает raw uint16_t без выделения памяти; для своих f8.8 payload. | `auto value = M::Float::encode(45.5f);` |

### Messages::IndexedValue

ID11/13/89/91/106/108: HB — индекс, LB — значение, оба 0..255. Чтение передаёт индекс в HB и ноль в LB; ответ повторяет индекс запроса. Для TSP/истории ошибок после получения ParameterCount.

Исходник: [IndexedValue.h](../src/Messages/IndexedValue.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `IndexedValue()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::IndexedValue m{};` |
| `explicit IndexedValue(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::IndexedValue m{snapshot};` |
| `explicit IndexedValue(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::IndexedValue m{result};` |
| `IndexedValue(Protocol::MessageID id, uint8_t index)` (public) | Создаёт READ_DATA: индекс в HB, LB=0, новая metadata. Для посимвольного/индексного чтения; id выбирает поддерживаемую группу. | `M::IndexedValue m{ID::TSPindexTSPvalue, 0};` |
| `IndexedValue( Protocol::MessageID id, uint8_t index, uint8_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, index, value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::IndexedValue m{ID::TSPindexTSPvalue, 0, 42, MT::WRITE_DATA};` |
| `uint8_t getIndex() const` (public) | Возвращает индекс записи, HB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getIndex();` |
| `uint8_t getValue() const` (public) | Возвращает значение записи, LB, 0..255; смысл задаёт производитель; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getValue();` |
| `IndexedValue reply(Protocol::MessageType type, uint8_t value) const` (public) | Тип явно передан. Повторяет текущий индекс HB, помещает value в LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 42);` |
| `static uint16_t encode(uint8_t index, uint8_t value)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 42);` |

### Messages::Int16

Знаковый payload в дополнительном коде, -32768..32767. Для s16, например ID33 температуры выхлопа в градусах Цельсия; масштабирования f8.8 нет.

Исходник: [Int16.h](../src/Messages/Int16.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Int16()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::Int16 m{};` |
| `explicit Int16(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Int16 m{snapshot};` |
| `explicit Int16(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Int16 m{result};` |
| `explicit Int16(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::Int16 m{ID::Texhaust};` |
| `Int16( Protocol::MessageID id, int16_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Int16 m{static_cast<ID>(200), 42, MT::WRITE_DATA};` |
| `int16_t getInt() const` (public) | Возвращает знаковый payload в дополнительном коде, -32768..32767; без проверки ACK/ID. Для декодирования успешного результата. | `auto value = m.getInt();` |
| `static Int16 read(Protocol::MessageID id)` (public) | Фабрика READ_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::Int16::read(ID::Texhaust);` |
| `static Int16 write(Protocol::MessageID id, int16_t value)` (public) | Фабрика WRITE_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::Int16::write(static_cast<ID>(200), 42);` |
| `Int16 reply(Protocol::MessageType type, int16_t value) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 42);` |

### Messages::MasterConfig

ID2: возможности master в HB, идентификатор производителя в LB. Для обмена возможностями или представления конфигурации master термостатом/прокси.

Исходник: [MasterConfig.h](../src/Messages/MasterConfig.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `MasterConfig()` (public) | Создаёт WRITE_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::MasterConfig m{};` |
| `explicit MasterConfig(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::MasterConfig m{snapshot};` |
| `explicit MasterConfig(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::MasterConfig m{result};` |
| `MasterConfig( uint8_t flags, uint8_t memberId, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей flags, memberId, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::MasterConfig m{0, 1, MT::WRITE_DATA};` |
| `bool isSmartPowerImplemented() const` (public) | Возвращает поддержка Smart Power master; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isSmartPowerImplemented();` |
| `uint8_t getMemberId() const` (public) | Возвращает идентификатор производителя/member, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMemberId();` |
| `MasterConfig& setSmartPowerImplemented(bool value)` (public) | Изменяет поддержка Smart Power master; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает MasterConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setSmartPowerImplemented(true);` |
| `MasterConfig& setMemberId(uint8_t memberId)` (public) | Изменяет идентификатор производителя/member, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает MasterConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setMemberId(1);` |
| `MasterConfig reply(Protocol::MessageType type, uint8_t flags, uint8_t memberId) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::WRITE_ACK, 0, 1);` |
| `MasterConfig reply(uint8_t flags, uint8_t memberId) const` (public) | Тип MT::WRITE_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 1);` |
| `static uint16_t encode(uint8_t flags, uint8_t memberId)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 1);` |

### Messages::NominalVentilation

ID87: номинальная вентиляция в процентах в HB, LB зарезервирован и в новых кадрах равен нулю. Отличается от UInt8 ID71/77/78 со значением в LB. Ограничения 0..100 нет.

Исходник: [NominalVentilation.h](../src/Messages/NominalVentilation.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `NominalVentilation()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::NominalVentilation m{};` |
| `explicit NominalVentilation(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::NominalVentilation m{snapshot};` |
| `explicit NominalVentilation(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::NominalVentilation m{result};` |
| `explicit NominalVentilation( uint8_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::NominalVentilation m{42, MT::WRITE_DATA};` |
| `uint8_t get() const` (public) | Возвращает беззнаковый HB номинальной вентиляции, проценты; без проверки ACK/ID. Для декодирования успешного результата. | `auto value = m.get();` |
| `NominalVentilation reply(Protocol::MessageType type, uint8_t value) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 42);` |

### Messages::ParameterCount

Размеры ID10/12/88/90/105/107: HB — беззнаковое количество записей, LB зарезервирован. Применяется перед обходом соответствующей группы IndexedValue TSP/истории ошибок. Количество не является максимальным индексом.

Исходник: [ParameterCount.h](../src/Messages/ParameterCount.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `ParameterCount()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::ParameterCount m{};` |
| `explicit ParameterCount(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::ParameterCount m{snapshot};` |
| `explicit ParameterCount(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::ParameterCount m{result};` |
| `explicit ParameterCount(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::ParameterCount m{ID::TSP};` |
| `ParameterCount( Protocol::MessageID id, uint8_t count, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей id, count, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::ParameterCount m{ID::TSP, 3, MT::READ_ACK};` |
| `uint8_t getCount() const` (public) | Возвращает количество записей, HB, 0..255; LB зарезервирован; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getCount();` |
| `ParameterCount reply(Protocol::MessageType type, uint8_t count) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 3);` |

### Messages::ProductVersion

ID76/104/126/127: тип продукта в HB, версия продукта в LB. Для идентификации вентиляции, солнечного накопителя, master/slave; это пара байтов, а не версия протокола OpenTherm f8.8.

Исходник: [ProductVersion.h](../src/Messages/ProductVersion.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `ProductVersion()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::ProductVersion m{};` |
| `explicit ProductVersion(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::ProductVersion m{snapshot};` |
| `explicit ProductVersion(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::ProductVersion m{result};` |
| `explicit ProductVersion(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::ProductVersion m{ID::SlaveVersion};` |
| `ProductVersion( Protocol::MessageID id, uint8_t productType, uint8_t version, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, productType, version, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::ProductVersion m{ID::SlaveVersion, 1, 2, MT::READ_ACK};` |
| `uint8_t getProductType() const` (public) | Возвращает тип продукта, HB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getProductType();` |
| `uint8_t getVersion() const` (public) | Возвращает версия продукта, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getVersion();` |
| `ProductVersion reply(Protocol::MessageType type, uint8_t productType, uint8_t version) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 1, 2);` |
| `static uint16_t encode(uint8_t productType, uint8_t version)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(1, 2);` |

### Messages::RFSensorStatus

ID98: тип/индекс RF-датчика в HB, батарея/сигнал в LB. Для передачи master состояния беспроводного датчика. Кодирование маскирует разрядность, обнуляет зарезервированные биты7..5 LB, не проверяет смысловой диапазон enum.

Исходник: [RFSensorStatus.h](../src/Messages/RFSensorStatus.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

#### RFSensorStatus::SensorType

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `ROOM_CONTROLLER` | 0 | комнатный контроллер |
| `ROOM_SENSOR` | 1 | комнатный датчик |
| `OUTSIDE_SENSOR` | 2 | наружный датчик |
| `UNDEFINED` | 15 | неопределённый тип датчика |

```cpp
auto value = M::RFSensorStatus::SensorType::ROOM_CONTROLLER;
```

#### RFSensorStatus::BatteryStatus

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `NO_INDICATION` | 0 | состояние батареи неизвестно |
| `LOW_BATTERY` | 1 | разряженная батарея |
| `NEARLY_LOW` | 2 | батарея близка к разряду |
| `NORMAL` | 3 | нормальная батарея |

```cpp
auto value = M::RFSensorStatus::BatteryStatus::NO_INDICATION;
```

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `RFSensorStatus()` (public) | Создаёт WRITE_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::RFSensorStatus m{};` |
| `explicit RFSensorStatus(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::RFSensorStatus m{snapshot};` |
| `explicit RFSensorStatus(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::RFSensorStatus m{result};` |
| `RFSensorStatus( SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей sensorType, sensorIndex, batteryStatus, signalStrength, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::RFSensorStatus m{M::RFSensorStatus::SensorType::ROOM_SENSOR, 0, M::RFSensorStatus::BatteryStatus::NORMAL, 5, MT::WRITE_DATA};` |
| `SensorType getSensorType() const` (public) | Возвращает тип RF-датчика, биты7..4 HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSensorType();` |
| `uint8_t getSensorIndex() const` (public) | Возвращает индекс RF-датчика, биты3..0 HB, 0..15; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSensorIndex();` |
| `BatteryStatus getBatteryStatus() const` (public) | Возвращает состояние батареи RF, биты1..0 LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getBatteryStatus();` |
| `uint8_t getSignalStrength() const` (public) | Возвращает уровень RF-сигнала, биты4..2 LB: 0 неизвестен, 1 слабый/потерянный, 5 отличный; прочие raw-коды сохраняются; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSignalStrength();` |
| `RFSensorStatus reply( Protocol::MessageType type, SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength ) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::WRITE_ACK, M::RFSensorStatus::SensorType::ROOM_SENSOR, 0, M::RFSensorStatus::BatteryStatus::NORMAL, 5);` |
| `static uint16_t encode( SensorType sensorType, uint8_t sensorIndex, BatteryStatus batteryStatus, uint8_t signalStrength )` (public) | Публичный статический кодировщик payload. Маскирует type/index до4 бит, battery до2, signal до3; обнуляет биты7..5 LB. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `auto value = M::RFSensorStatus::encode(M::RFSensorStatus::SensorType::ROOM_SENSOR, 0, M::RFSensorStatus::BatteryStatus::NORMAL, 5);` |

### Messages::RemoteOverrideFunction

ID100: биты0/1 LB — приоритет ручных/программных изменений согласно подробной таблице спецификации; обзорная таблица противоречит ей. Остальные биты payload сохраняются setters и reply.

Исходник: [RemoteOverrideFunction.h](../src/Messages/RemoteOverrideFunction.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `RemoteOverrideFunction()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::RemoteOverrideFunction m{};` |
| `explicit RemoteOverrideFunction(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::RemoteOverrideFunction m{snapshot};` |
| `explicit RemoteOverrideFunction(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::RemoteOverrideFunction m{result};` |
| `bool isManualChangePriorityEnabled() const` (public) | Возвращает приоритет ручных изменений master; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isManualChangePriorityEnabled();` |
| `bool isProgramChangePriorityEnabled() const` (public) | Возвращает приоритет программных изменений master; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0002. | `auto value = m.isProgramChangePriorityEnabled();` |
| `RemoteOverrideFunction& setManualChangePriorityEnabled(bool value)` (public) | Изменяет приоритет ручных изменений master; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteOverrideFunction& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setManualChangePriorityEnabled(true);` |
| `RemoteOverrideFunction& setProgramChangePriorityEnabled(bool value)` (public) | Изменяет приоритет программных изменений master; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteOverrideFunction& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0002. | `m.setProgramChangePriorityEnabled(true);` |
| `RemoteOverrideFunction reply(bool manualChangePriority, bool programChangePriority) const` (public) | Тип MT::READ_ACK. Сохраняет весь payload, кроме двух задаваемых битов приоритета LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(true, true);` |
| `RemoteOverrideFunction reply( Protocol::MessageType type, bool manualChangePriority, bool programChangePriority ) const` (public) | Тип явно передан. Сохраняет весь payload, кроме двух задаваемых битов приоритета LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, true, true);` |

### Messages::RemoteOverrideMode

ID99: CH1 — биты3..0 LB, CH2 — биты7..4 LB, ГВС — биты3..0 HB, принудительное ГВС — бит4 HB. Для удалённого выбора режима; значения 2 и 3 enum отопления и ГВС имеют разный смысл.

Исходник: [RemoteOverrideMode.h](../src/Messages/RemoteOverrideMode.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

#### RemoteOverrideMode::HeatingMode

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `NO_OVERRIDE` | 0 | без удалённого переопределения |
| `AUTO` | 1 | автоматический/программный режим |
| `COMFORT` | 2 | комфортный режим |
| `PRECOMFORT` | 3 | предкомфортный режим |
| `REDUCED` | 4 | сниженный режим |
| `PROTECTION` | 5 | защитный режим |
| `OFF` | 6 | выключено |

```cpp
auto value = M::RemoteOverrideMode::HeatingMode::NO_OVERRIDE;
```

#### RemoteOverrideMode::DhwMode

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `NO_OVERRIDE` | 0 | без удалённого переопределения |
| `AUTO` | 1 | автоматический/программный режим |
| `ANTI_LEGIONELLA` | 2 | анти-легионелла |
| `COMFORT` | 3 | комфортный режим |
| `REDUCED` | 4 | сниженный режим |
| `PROTECTION` | 5 | защитный режим |
| `OFF` | 6 | выключено |

```cpp
auto value = M::RemoteOverrideMode::DhwMode::NO_OVERRIDE;
```

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `RemoteOverrideMode()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::RemoteOverrideMode m{};` |
| `explicit RemoteOverrideMode(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::RemoteOverrideMode m{snapshot};` |
| `explicit RemoteOverrideMode(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::RemoteOverrideMode m{result};` |
| `RemoteOverrideMode( HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей heatingMode, ch2Mode, dhwMode, dhwPush, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::RemoteOverrideMode m{M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::DhwMode::AUTO, false, MT::WRITE_DATA};` |
| `HeatingMode getHeatingMode() const` (public) | Возвращает режим удалённого управления CH1, биты3..0 LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getHeatingMode();` |
| `HeatingMode getCh2Mode() const` (public) | Возвращает режим удалённого управления CH2, биты7..4 LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getCh2Mode();` |
| `DhwMode getDhwMode() const` (public) | Возвращает режим удалённого управления ГВС, биты3..0 HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getDhwMode();` |
| `bool isDhwPushRequested() const` (public) | Возвращает запрос разового принудительного ГВС, бит4 HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x1000. | `auto value = m.isDhwPushRequested();` |
| `RemoteOverrideMode reply( Protocol::MessageType type, HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false ) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::DhwMode::AUTO, false);` |
| `static uint16_t encode(HeatingMode heatingMode, HeatingMode ch2Mode, DhwMode dhwMode, bool dhwPush = false)` (public) | Публичный статический кодировщик payload. Маскирует каждый режим до4 бит, dhwPush в bit12; обнуляет прочие reserved-биты HB. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `auto value = M::RemoteOverrideMode::encode(M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::HeatingMode::AUTO, M::RemoteOverrideMode::DhwMode::AUTO, false);` |

### Messages::RemoteParameters

ID6: разрешение передачи/поддержка в HB, разрешение записи в LB. Проверяются перед записью ID56 уставки ГВС и ID57 максимальной уставки отопления; поддержка и право записи независимы.

Исходник: [RemoteParameters.h](../src/Messages/RemoteParameters.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `RemoteParameters()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::RemoteParameters m{};` |
| `explicit RemoteParameters(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::RemoteParameters m{snapshot};` |
| `explicit RemoteParameters(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::RemoteParameters m{result};` |
| `RemoteParameters( uint8_t transferEnableFlags, uint8_t readWriteFlags, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей transferEnableFlags, readWriteFlags, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::RemoteParameters m{0, 0, MT::READ_ACK};` |
| `bool isDhwSetpointSupported() const` (public) | Возвращает поддержка/передача уставки ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isDhwSetpointSupported();` |
| `bool isDhwSetpointWritable() const` (public) | Возвращает право записи уставки ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isDhwSetpointWritable();` |
| `bool isMaxHeatingSetpointSupported() const` (public) | Возвращает поддержка/передача максимальной уставки отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.isMaxHeatingSetpointSupported();` |
| `bool isMaxHeatingSetpointWritable() const` (public) | Возвращает право записи максимальной уставки отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0002. | `auto value = m.isMaxHeatingSetpointWritable();` |
| `RemoteParameters& setDhwSetpointSupported(bool value)` (public) | Изменяет поддержка/передача уставки ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setDhwSetpointSupported(true);` |
| `RemoteParameters& setDhwSetpointWritable(bool value)` (public) | Изменяет право записи уставки ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setDhwSetpointWritable(true);` |
| `RemoteParameters& setMaxHeatingSetpointSupported(bool value)` (public) | Изменяет поддержка/передача максимальной уставки отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setMaxHeatingSetpointSupported(true);` |
| `RemoteParameters& setMaxHeatingSetpointWritable(bool value)` (public) | Изменяет право записи максимальной уставки отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает RemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0002. | `m.setMaxHeatingSetpointWritable(true);` |
| `RemoteParameters reply(Protocol::MessageType type, uint8_t transferEnableFlags, uint8_t readWriteFlags) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 0);` |
| `RemoteParameters reply(uint8_t transferEnableFlags, uint8_t readWriteFlags) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 0);` |
| `static uint16_t encode(uint8_t transferEnableFlags, uint8_t readWriteFlags)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 0);` |

### Messages::RemoteRequest

ID4: код команды в HB, код ответа/завершения в LB. Для поддерживаемых устройством удалённых команд, например документированного снятия блокировки; wrapper сам не выполняет и не разрешает команду.

Исходник: [RemoteRequest.h](../src/Messages/RemoteRequest.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `RemoteRequest()` (public) | Создаёт WRITE_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::RemoteRequest m{};` |
| `explicit RemoteRequest(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::RemoteRequest m{snapshot};` |
| `explicit RemoteRequest(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::RemoteRequest m{result};` |
| `explicit RemoteRequest( uint8_t requestCode, uint8_t responseCode = 0, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей requestCode, responseCode, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::RemoteRequest m{0, 128, MT::WRITE_DATA};` |
| `uint8_t getRequestCode() const` (public) | Возвращает код удалённой команды, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getRequestCode();` |
| `uint8_t getResponseCode() const` (public) | Возвращает код завершения удалённой команды, LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getResponseCode();` |
| `bool isRequestAccepted() const` (public) | True только при WRITE_ACK и responseCode>=128. Это условие кода завершения ID4, не просто isSuccess; проверяется после получения ответа удалённой команды. | `auto value = m.isRequestAccepted();` |
| `RemoteRequest reply(Protocol::MessageType type, uint8_t responseCode) const` (public) | Тип явно передан. Повторяет код команды HB, заменяет код ответа LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::WRITE_ACK, 128);` |
| `static uint16_t encode(uint8_t requestCode, uint8_t responseCode)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 128);` |

### Messages::SlaveConfig

ID3: возможности slave в HB, идентификатор производителя в LB. Для определения ГВС, охлаждения, CH2 и режимов регулирования перед выбором поведения приложения.

Исходник: [SlaveConfig.h](../src/Messages/SlaveConfig.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `SlaveConfig()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::SlaveConfig m{};` |
| `explicit SlaveConfig(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::SlaveConfig m{snapshot};` |
| `explicit SlaveConfig(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::SlaveConfig m{result};` |
| `SlaveConfig( uint8_t flags, uint8_t memberId, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей flags, memberId, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::SlaveConfig m{0, 1, MT::READ_ACK};` |
| `bool isDhwPresent() const` (public) | Возвращает наличие ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isDhwPresent();` |
| `bool isOnOffControl() const` (public) | Возвращает двухпозиционное управление; false означает модуляционное; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.isOnOffControl();` |
| `bool isCoolingSupported() const` (public) | Возвращает поддержка охлаждения; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.isCoolingSupported();` |
| `bool hasDhwStorageTank() const` (public) | Возвращает накопительный бак ГВС; false означает проточный/неуказанный вариант; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0800. | `auto value = m.hasDhwStorageTank();` |
| `bool isMasterLowOffPumpControlAllowed() const` (public) | Возвращает разрешение master управлять насосом low/off; бит payload инверсный; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x1000. | `auto value = m.isMasterLowOffPumpControlAllowed();` |
| `bool isCh2Present() const` (public) | Возвращает наличие второго контура отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x2000. | `auto value = m.isCh2Present();` |
| `bool isRemoteWaterFillingUnavailable() const` (public) | Возвращает недоступность удалённого заполнения; false означает доступность или неизвестность; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x4000. | `auto value = m.isRemoteWaterFillingUnavailable();` |
| `bool isHeatCoolModeControlledBySlave() const` (public) | Возвращает выбор режима нагрев/охлаждение самим slave; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x8000. | `auto value = m.isHeatCoolModeControlledBySlave();` |
| `uint8_t getMemberId() const` (public) | Возвращает идентификатор производителя/member, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMemberId();` |
| `SlaveConfig& setDhwPresent(bool value)` (public) | Изменяет наличие ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setDhwPresent(true);` |
| `SlaveConfig& setOnOffControl(bool value)` (public) | Изменяет двухпозиционное управление; false означает модуляционное; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setOnOffControl(true);` |
| `SlaveConfig& setCoolingSupported(bool value)` (public) | Изменяет поддержка охлаждения; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setCoolingSupported(true);` |
| `SlaveConfig& setDhwStorageTank(bool value)` (public) | Изменяет накопительный бак ГВС; false означает проточный/неуказанный вариант; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0800. | `m.setDhwStorageTank(true);` |
| `SlaveConfig& setMasterLowOffPumpControlAllowed(bool value)` (public) | Изменяет разрешение master управлять насосом low/off; бит payload инверсный; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x1000. | `m.setMasterLowOffPumpControlAllowed(true);` |
| `SlaveConfig& setCh2Present(bool value)` (public) | Изменяет наличие второго контура отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x2000. | `m.setCh2Present(true);` |
| `SlaveConfig& setRemoteWaterFillingUnavailable(bool value)` (public) | Изменяет недоступность удалённого заполнения; false означает доступность или неизвестность; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x4000. | `m.setRemoteWaterFillingUnavailable(true);` |
| `SlaveConfig& setHeatCoolModeControlledBySlave(bool value)` (public) | Изменяет выбор режима нагрев/охлаждение самим slave; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x8000. | `m.setHeatCoolModeControlledBySlave(true);` |
| `SlaveConfig& setMemberId(uint8_t memberId)` (public) | Изменяет идентификатор производителя/member, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SlaveConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setMemberId(1);` |
| `SlaveConfig reply(Protocol::MessageType type, uint8_t flags, uint8_t memberId) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 1);` |
| `SlaveConfig reply(uint8_t flags, uint8_t memberId) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 1);` |
| `static uint16_t encode(uint8_t flags, uint8_t memberId)` (private) | Внутренний private helper. Упаковывает поля в описанный выше HB/LB, без смысловой проверки диапазонов. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(0, 1);` |

### Messages::SolarStorageConfig

ID103: байт конфигурации в HB, member ID в LB. Описание бита типа системы противоречит расположению байтов, поэтому API предоставляет байты без предположения о бите. Для трактовки конфигурации нужна документация производителя.

Исходник: [SolarStorageConfig.h](../src/Messages/SolarStorageConfig.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `SolarStorageConfig()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::SolarStorageConfig m{};` |
| `explicit SolarStorageConfig(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::SolarStorageConfig m{snapshot};` |
| `explicit SolarStorageConfig(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::SolarStorageConfig m{result};` |
| `uint8_t getConfigurationByte() const` (public) | Возвращает неинтерпретированный байт конфигурации, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getConfigurationByte();` |
| `uint8_t getMemberId() const` (public) | Возвращает идентификатор производителя/member, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMemberId();` |
| `SolarStorageConfig& setConfigurationByte(uint8_t configuration)` (public) | Изменяет неинтерпретированный байт конфигурации, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setConfigurationByte(0);` |
| `SolarStorageConfig& setMemberId(uint8_t memberId)` (public) | Изменяет идентификатор производителя/member, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setMemberId(1);` |
| `SolarStorageConfig reply(uint8_t configuration, uint8_t memberId) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 1);` |
| `SolarStorageConfig reply(Protocol::MessageType type, uint8_t configuration, uint8_t memberId) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 1);` |

### Messages::SolarStorageFault

ID102: определён только OEM-код в LB; флаги HB зарезервированы. Для диагностики солнечного накопителя. Изменение кода и ответ сохраняют HB; флаги ошибок котла не приписываются.

Исходник: [SolarStorageFault.h](../src/Messages/SolarStorageFault.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `SolarStorageFault()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::SolarStorageFault m{};` |
| `explicit SolarStorageFault(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::SolarStorageFault m{snapshot};` |
| `explicit SolarStorageFault(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::SolarStorageFault m{result};` |
| `uint8_t getOemFaultCode() const` (public) | Возвращает код неисправности производителя, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getOemFaultCode();` |
| `SolarStorageFault& setOemFaultCode(uint8_t code)` (public) | Изменяет код неисправности производителя, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageFault& для цепочки; для редактирования локального сообщения/ответа. | `m.setOemFaultCode(0);` |
| `SolarStorageFault reply(uint8_t code) const` (public) | Тип MT::READ_ACK. Сохраняет зарезервированный HB и заменяет только OEM-код LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0);` |
| `SolarStorageFault reply(Protocol::MessageType type, uint8_t code) const` (public) | Тип явно передан. Сохраняет зарезервированный HB и заменяет только OEM-код LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0);` |

### Messages::SolarStorageStatus

ID101: запрос режима master — биты2..0 HB; ошибка — бит0 LB, фактический режим — биты3..1 LB, состояние — биты5..4 LB. Для солнечного накопителя, без флагов Status котла. Ответ повторяет весь HB и сохраняет биты7..6 LB.

Исходник: [SolarStorageStatus.h](../src/Messages/SolarStorageStatus.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

#### SolarStorageStatus::Mode

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `OFF` | 0 | выключено |
| `DHW_ECO` | 1 | экономичный ГВС |
| `DHW_COMFORT` | 2 | комфортный ГВС |
| `DHW_SINGLE_BOOST` | 3 | разовое усиление ГВС |
| `DHW_CONTINUOUS_BOOST` | 4 | постоянное усиление ГВС |

```cpp
auto value = M::SolarStorageStatus::Mode::OFF;
```

#### SolarStorageStatus::State

Публичный enum class : uint8_t. Для одноимённых полей этого wrapper; неизвестные raw-коды при parse не отклоняются.

| Значение | Код | Смысл |
| --- | ---: | --- |
| `STANDBY` | 0 | ожидание |
| `SOLAR_LOADING` | 1 | нагрев от солнечного источника |
| `BOILER_LOADING` | 2 | нагрев от котла |
| `ANTI_LEGIONELLA` | 3 | анти-легионелла |

```cpp
auto value = M::SolarStorageStatus::State::STANDBY;
```

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `SolarStorageStatus()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload (Mode::OFF), новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::SolarStorageStatus m{};` |
| `explicit SolarStorageStatus(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::SolarStorageStatus m{snapshot};` |
| `explicit SolarStorageStatus(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::SolarStorageStatus m{result};` |
| `explicit SolarStorageStatus(Mode masterMode)` (public) | Создаёт новый кадр из полей masterMode; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::SolarStorageStatus m{M::SolarStorageStatus::Mode::DHW_ECO};` |
| `Mode getMasterMode() const` (public) | Возвращает запрашиваемый режим солнечного накопителя, биты2..0 HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMasterMode();` |
| `Mode getSlaveMode() const` (public) | Возвращает фактический режим солнечного накопителя, биты3..1 LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSlaveMode();` |
| `State getState() const` (public) | Возвращает рабочее состояние солнечного накопителя, биты5..4 LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getState();` |
| `bool isFault() const` (public) | Возвращает признак неисправности slave; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isFault();` |
| `SolarStorageStatus& setMasterMode(Mode mode)` (public) | Изменяет запрашиваемый режим солнечного накопителя, биты2..0 HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageStatus& для цепочки; для редактирования локального сообщения/ответа. | `m.setMasterMode(M::SolarStorageStatus::Mode::DHW_ECO);` |
| `SolarStorageStatus& setSlaveMode(Mode mode)` (public) | Изменяет фактический режим солнечного накопителя, биты3..1 LB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageStatus& для цепочки; для редактирования локального сообщения/ответа. | `m.setSlaveMode(M::SolarStorageStatus::Mode::DHW_ECO);` |
| `SolarStorageStatus& setState(State state)` (public) | Изменяет рабочее состояние солнечного накопителя, биты5..4 LB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageStatus& для цепочки; для редактирования локального сообщения/ответа. | `m.setState(M::SolarStorageStatus::State::STANDBY);` |
| `SolarStorageStatus& setFault(bool value)` (public) | Изменяет признак неисправности slave; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает SolarStorageStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setFault(true);` |
| `SolarStorageStatus reply(Mode slaveMode, State state, bool fault = false) const` (public) | Тип MT::READ_ACK. Повторяет весь HB и сохраняет биты7..6 LB; заменяет режим/state/fault slave. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(M::SolarStorageStatus::Mode::DHW_ECO, M::SolarStorageStatus::State::STANDBY, false);` |
| `SolarStorageStatus reply( Protocol::MessageType type, Mode slaveMode, State state, bool fault = false ) const` (public) | Тип явно передан. Повторяет весь HB и сохраняет биты7..6 LB; заменяет режим/state/fault slave. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, M::SolarStorageStatus::Mode::DHW_ECO, M::SolarStorageStatus::State::STANDBY, false);` |

### Messages::Status

Только ID0: HB — запрашиваемые флаги master, LB — фактическое состояние slave. Разрешение отопления/ГВС отделено от активности и пламени. Setters изменяют сообщение в памяти; отправка выполняется отдельно.

Исходник: [Status.h](../src/Messages/Status.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `Status()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::Status m{};` |
| `explicit Status(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::Status m{snapshot};` |
| `explicit Status(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::Status m{result};` |
| `explicit Status(uint8_t masterFlags, uint8_t lowByte = 0)` (public) | Создаёт новый кадр из полей masterFlags, lowByte; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::Status m{0, 0};` |
| `static Status request(uint8_t masterFlags = 0, uint8_t lowByte = 0)` (public) | Фабрика Status READ_DATA ID0 из masterFlags(HB) и lowByte(LB); эквивалент конструктора, metadata новая. Для периодического Status master. | `auto value = M::Status::request(0, 0);` |
| `Status reply(uint8_t slaveFlags) const` (public) | Тип MT::READ_ACK. Повторяет весь master HB; заменяет slave LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0);` |
| `Status reply(Protocol::MessageType type, uint8_t slaveFlags) const` (public) | Тип явно передан. Повторяет весь master HB; заменяет slave LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0);` |
| `uint8_t getMasterFlags() const` (public) | Возвращает байт запрашиваемых флагов master, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMasterFlags();` |
| `uint8_t getSlaveFlags() const` (public) | Возвращает байт фактического состояния slave, LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSlaveFlags();` |
| `bool isCentralHeatingEnabled() const` (public) | Возвращает запрос разрешения отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isCentralHeatingEnabled();` |
| `bool isHotWaterEnabled() const` (public) | Возвращает запрос разрешения ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.isHotWaterEnabled();` |
| `bool isCoolingEnabled() const` (public) | Возвращает запрос разрешения охлаждения; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.isCoolingEnabled();` |
| `bool isOutsideTemperatureCompensationEnabled() const` (public) | Возвращает запрос разрешения погодозависимой компенсации; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0800. | `auto value = m.isOutsideTemperatureCompensationEnabled();` |
| `bool isCh2Enabled() const` (public) | Возвращает запрос разрешения второго контура отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x1000. | `auto value = m.isCh2Enabled();` |
| `bool isSummerWinterMode() const` (public) | Возвращает запрос летнего/зимнего режима; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x2000. | `auto value = m.isSummerWinterMode();` |
| `bool isDhwBlocking() const` (public) | Возвращает запрос блокирования ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x4000. | `auto value = m.isDhwBlocking();` |
| `bool isFault() const` (public) | Возвращает признак неисправности slave; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isFault();` |
| `bool isCentralHeatingActive() const` (public) | Возвращает фактическая активность отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0002. | `auto value = m.isCentralHeatingActive();` |
| `bool isHotWaterActive() const` (public) | Возвращает фактическая активность ГВС; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0004. | `auto value = m.isHotWaterActive();` |
| `bool isFlameOn() const` (public) | Возвращает признак пламени; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0008. | `auto value = m.isFlameOn();` |
| `bool isCoolingActive() const` (public) | Возвращает фактическая активность охлаждения; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0010. | `auto value = m.isCoolingActive();` |
| `bool isCh2Active() const` (public) | Возвращает фактическая активность второго контура отопления; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0020. | `auto value = m.isCh2Active();` |
| `bool isDiagnostic() const` (public) | Возвращает признак диагностического события; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0040. | `auto value = m.isDiagnostic();` |
| `bool isElectricityProductionActive() const` (public) | Возвращает фактическая активность производства электроэнергии; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0080. | `auto value = m.isElectricityProductionActive();` |
| `Status& setMasterFlags(uint8_t flags)` (public) | Изменяет байт запрашиваемых флагов master, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Для Status whole-byte setter ID пересобирается как ID0. | `m.setMasterFlags(0);` |
| `Status& setSlaveFlags(uint8_t flags)` (public) | Изменяет байт фактического состояния slave, LB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Для Status whole-byte setter ID пересобирается как ID0. | `m.setSlaveFlags(0);` |
| `Status& setCentralHeatingEnabled(bool value)` (public) | Изменяет запрос разрешения отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setCentralHeatingEnabled(true);` |
| `Status& setHotWaterEnabled(bool value)` (public) | Изменяет запрос разрешения ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setHotWaterEnabled(true);` |
| `Status& setCoolingEnabled(bool value)` (public) | Изменяет запрос разрешения охлаждения; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setCoolingEnabled(true);` |
| `Status& setOutsideTemperatureCompensationEnabled(bool value)` (public) | Изменяет запрос разрешения погодозависимой компенсации; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0800. | `m.setOutsideTemperatureCompensationEnabled(true);` |
| `Status& setCh2Enabled(bool value)` (public) | Изменяет запрос разрешения второго контура отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x1000. | `m.setCh2Enabled(true);` |
| `Status& setSummerWinterMode(bool value)` (public) | Изменяет запрос летнего/зимнего режима; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x2000. | `m.setSummerWinterMode(true);` |
| `Status& setDhwBlocking(bool value)` (public) | Изменяет запрос блокирования ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x4000. | `m.setDhwBlocking(true);` |
| `Status& setFault(bool value)` (public) | Изменяет признак неисправности slave; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setFault(true);` |
| `Status& setCentralHeatingActive(bool value)` (public) | Изменяет фактическая активность отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0002. | `m.setCentralHeatingActive(true);` |
| `Status& setHotWaterActive(bool value)` (public) | Изменяет фактическая активность ГВС; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0004. | `m.setHotWaterActive(true);` |
| `Status& setFlameOn(bool value)` (public) | Изменяет признак пламени; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0008. | `m.setFlameOn(true);` |
| `Status& setCoolingActive(bool value)` (public) | Изменяет фактическая активность охлаждения; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0010. | `m.setCoolingActive(true);` |
| `Status& setCh2Active(bool value)` (public) | Изменяет фактическая активность второго контура отопления; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0020. | `m.setCh2Active(true);` |
| `Status& setDiagnostic(bool value)` (public) | Изменяет признак диагностического события; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0040. | `m.setDiagnostic(true);` |
| `Status& setElectricityProductionActive(bool value)` (public) | Изменяет фактическая активность производства электроэнергии; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает Status& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0080. | `m.setElectricityProductionActive(true);` |
| `Status& setMasterFlag(uint8_t mask, bool value)` (private) | Private helper Status: установить/сбросить mask в HB, сохранить остальные флаги, вернуть *this. Вызывается именованными setters; не доступен приложению. | `Внутри реализации: setMasterFlag(0x01, true);` |
| `Status& setSlaveFlag(uint8_t mask, bool value)` (private) | Private helper Status: установить/сбросить mask в LB, сохранить остальные флаги, вернуть *this. Вызывается именованными setters; не доступен приложению. | `Внутри реализации: setSlaveFlag(0x01, true);` |

### Messages::TemperatureBounds

ID48/49: знаковый 8-битный верхний предел в HB, нижний в LB, градусы Цельсия. Для ограничений уставок ГВС/максимальной температуры отопления; каждый байт целый -128..127, а не f8.8.

Исходник: [TemperatureBounds.h](../src/Messages/TemperatureBounds.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `TemperatureBounds()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::TemperatureBounds m{};` |
| `explicit TemperatureBounds(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::TemperatureBounds m{snapshot};` |
| `explicit TemperatureBounds(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::TemperatureBounds m{result};` |
| `explicit TemperatureBounds(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::TemperatureBounds m{ID::TdhwSetUBTdhwSetLB};` |
| `TemperatureBounds( Protocol::MessageID id, int8_t upper, int8_t lower, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей id, upper, lower, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::TemperatureBounds m{ID::TdhwSetUBTdhwSetLB, 60, 30, MT::READ_ACK};` |
| `int8_t getUpper() const` (public) | Возвращает верхняя знаковая граница температуры, HB, градусы Цельсия; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getUpper();` |
| `int8_t getLower() const` (public) | Возвращает нижняя знаковая граница температуры, LB, градусы Цельсия; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getLower();` |
| `TemperatureBounds reply(Protocol::MessageType type, int8_t upper, int8_t lower) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 60, 30);` |
| `static uint16_t encode(int8_t upper, int8_t lower)` (private) | Внутренний private helper. Преобразует каждый знаковый байт в raw8-битное представление перед упаковкой. Возвращает uint16_t; используется конструкторами/reply или при построении payload. | `Внутри реализации: encode(60, 30);` |
| `static int8_t decode(uint8_t value)` (private) | Private helper: декодирует uint8_t в signed -128..127, вычитая256 при bit7=1. Внутри getUpper/getLower, для корректных отрицательных температур. | `Внутри реализации: decode(42);` |

### Messages::UInt16

Беззнаковый payload целиком, 0..65535. Для счётчиков, часов, диагностических кодов, года, оборотов или мощности, если выбранный ID имеет формат u16.

Исходник: [UInt16.h](../src/Messages/UInt16.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `UInt16()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::UInt16 m{};` |
| `explicit UInt16(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::UInt16 m{snapshot};` |
| `explicit UInt16(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::UInt16 m{result};` |
| `explicit UInt16(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::UInt16 m{ID::Year};` |
| `UInt16( Protocol::MessageID id, uint16_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::UInt16 m{ID::Year, 42, MT::WRITE_DATA};` |
| `uint16_t getUInt() const` (public) | Возвращает беззнаковый payload, 0..65535; без проверки ACK/ID. Для декодирования успешного результата. | `auto value = m.getUInt();` |
| `static UInt16 read(Protocol::MessageID id)` (public) | Фабрика READ_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::UInt16::read(ID::Year);` |
| `static UInt16 write(Protocol::MessageID id, uint16_t value)` (public) | Фабрика WRITE_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::UInt16::write(ID::Year, 42);` |
| `UInt16 reply(Protocol::MessageType type, uint16_t value) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 42);` |

### Messages::UInt8

Беззнаковое значение LB, 0..255; новые payload обнуляют HB. Подходит для ID71 Vset, ID77 вентиляции и ID78 влажности вытяжного воздуха. При разборе полученный HB сохраняется.

Исходник: [UInt8.h](../src/Messages/UInt8.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `UInt8()` (public) | Объект с нулевым raw, status=NONE, hasFrame=false; конкретный ID не выбирается. Для последующего parse/присваивания, не готовый запрос этого формата. | `M::UInt8 m{};` |
| `explicit UInt8(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::UInt8 m{snapshot};` |
| `explicit UInt8(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::UInt8 m{result};` |
| `explicit UInt8(Protocol::MessageID id)` (public) | Создаёт READ_DATA выбранного id с payload0 и новой metadata; для чтения значения формата этого wrapper. | `M::UInt8 m{ID::RelVentLevel};` |
| `UInt8( Protocol::MessageID id, uint8_t value, Protocol::MessageType type = Protocol::MessageType::WRITE_DATA )` (public) | Создаёт новый кадр из полей id, value, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::UInt8 m{ID::Vset, 42, MT::WRITE_DATA};` |
| `uint8_t get() const` (public) | Возвращает беззнаковый LB, 0..255; полученный HB игнорируется, но сохраняется; без проверки ACK/ID. Для декодирования успешного результата. | `auto value = m.get();` |
| `static UInt8 read(Protocol::MessageID id)` (public) | Фабрика READ_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::UInt8::read(ID::RelVentLevel);` |
| `static UInt8 write(Protocol::MessageID id, uint8_t value)` (public) | Фабрика WRITE_DATA выбранного id с данными формата этого wrapper и новой NONE metadata; поддержка операции устройством не проверяется. Для последующей отправки через facade. | `auto value = M::UInt8::write(ID::Vset, 42);` |
| `UInt8 reply(Protocol::MessageType type, uint8_t value) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 42);` |

### Messages::VentilationConfig

ID74: конфигурация в HB, member ID в LB. Для определения вытяжной вентиляции/рекуперации, наличия байпаса и трёхскоростного/плавного регулирования.

Исходник: [VentilationConfig.h](../src/Messages/VentilationConfig.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `VentilationConfig()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::VentilationConfig m{};` |
| `explicit VentilationConfig(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::VentilationConfig m{snapshot};` |
| `explicit VentilationConfig(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::VentilationConfig m{result};` |
| `VentilationConfig( uint8_t configFlags, uint8_t memberId, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей configFlags, memberId, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::VentilationConfig m{0, 1, MT::READ_ACK};` |
| `uint8_t getConfigFlags() const` (public) | Возвращает байт флагов конфигурации, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getConfigFlags();` |
| `uint8_t getMemberId() const` (public) | Возвращает идентификатор производителя/member, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMemberId();` |
| `bool isHeatRecoverySystem() const` (public) | Возвращает система рекуперации; false означает центральную вытяжку; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isHeatRecoverySystem();` |
| `bool hasBypass() const` (public) | Возвращает наличие байпаса; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.hasBypass();` |
| `bool isVariableSpeed() const` (public) | Возвращает плавное регулирование скорости; false означает три скорости; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.isVariableSpeed();` |
| `VentilationConfig& setConfigFlags(uint8_t flags)` (public) | Изменяет байт флагов конфигурации, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setConfigFlags(0);` |
| `VentilationConfig& setMemberId(uint8_t memberId)` (public) | Изменяет идентификатор производителя/member, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationConfig& для цепочки; для редактирования локального сообщения/ответа. | `m.setMemberId(1);` |
| `VentilationConfig& setHeatRecoverySystem(bool value)` (public) | Изменяет система рекуперации; false означает центральную вытяжку; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setHeatRecoverySystem(true);` |
| `VentilationConfig& setBypassPresent(bool value)` (public) | Изменяет наличие байпаса; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setBypassPresent(true);` |
| `VentilationConfig& setVariableSpeed(bool value)` (public) | Изменяет плавное регулирование скорости; false означает три скорости; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationConfig& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setVariableSpeed(true);` |
| `VentilationConfig reply(uint8_t configFlags, uint8_t memberId) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 1);` |
| `VentilationConfig reply(Protocol::MessageType type, uint8_t configFlags, uint8_t memberId) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 1);` |

### Messages::VentilationFault

ID72: флаги неисправностей вентиляции в HB, OEM-код в LB. Для диагностики вентиляторов, обслуживания и защиты от обмерзания без переноса значений ID5 котла.

Исходник: [VentilationFault.h](../src/Messages/VentilationFault.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `VentilationFault()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::VentilationFault m{};` |
| `explicit VentilationFault(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::VentilationFault m{snapshot};` |
| `explicit VentilationFault(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::VentilationFault m{result};` |
| `VentilationFault( uint8_t faultFlags, uint8_t oemFaultCode, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей faultFlags, oemFaultCode, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::VentilationFault m{0, 0, MT::READ_ACK};` |
| `uint8_t getFaultFlags() const` (public) | Возвращает байт флагов неисправностей вентиляции, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getFaultFlags();` |
| `uint8_t getOemFaultCode() const` (public) | Возвращает код неисправности производителя, LB, 0..255; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getOemFaultCode();` |
| `bool isServiceRequired() const` (public) | Возвращает требование обслуживания; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isServiceRequired();` |
| `bool hasExhaustFanFault() const` (public) | Возвращает ошибка вытяжного вентилятора; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.hasExhaustFanFault();` |
| `bool hasInletFanFault() const` (public) | Возвращает ошибка приточного вентилятора; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.hasInletFanFault();` |
| `bool isFrostProtectionActive() const` (public) | Возвращает активная защита от обмерзания; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0800. | `auto value = m.isFrostProtectionActive();` |
| `VentilationFault& setFaultFlags(uint8_t flags)` (public) | Изменяет байт флагов неисправностей вентиляции, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. | `m.setFaultFlags(0);` |
| `VentilationFault& setOemFaultCode(uint8_t code)` (public) | Изменяет код неисправности производителя, LB, 0..255; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. | `m.setOemFaultCode(0);` |
| `VentilationFault& setServiceRequired(bool value)` (public) | Изменяет требование обслуживания; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setServiceRequired(true);` |
| `VentilationFault& setExhaustFanFault(bool value)` (public) | Изменяет ошибка вытяжного вентилятора; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setExhaustFanFault(true);` |
| `VentilationFault& setInletFanFault(bool value)` (public) | Изменяет ошибка приточного вентилятора; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setInletFanFault(true);` |
| `VentilationFault& setFrostProtectionActive(bool value)` (public) | Изменяет активная защита от обмерзания; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationFault& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0800. | `m.setFrostProtectionActive(true);` |
| `VentilationFault reply(uint8_t faultFlags, uint8_t oemFaultCode) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 0);` |
| `VentilationFault reply(Protocol::MessageType type, uint8_t faultFlags, uint8_t oemFaultCode) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 0);` |

### Messages::VentilationRemoteParameters

ID86: разрешения передачи в HB, чтения/записи в LB для номинальной вентиляции. Для проверки доступности и права записи ID87 перед изменением приложением.

Исходник: [VentilationRemoteParameters.h](../src/Messages/VentilationRemoteParameters.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `VentilationRemoteParameters()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::VentilationRemoteParameters m{};` |
| `explicit VentilationRemoteParameters(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::VentilationRemoteParameters m{snapshot};` |
| `explicit VentilationRemoteParameters(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::VentilationRemoteParameters m{result};` |
| `VentilationRemoteParameters( uint8_t transferFlags, uint8_t readWriteFlags, Protocol::MessageType type = Protocol::MessageType::READ_ACK )` (public) | Создаёт новый кадр из полей transferFlags, readWriteFlags, type; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::VentilationRemoteParameters m{0, 0, MT::READ_ACK};` |
| `uint8_t getTransferFlags() const` (public) | Возвращает байт разрешений передачи параметров, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getTransferFlags();` |
| `uint8_t getReadWriteFlags() const` (public) | Возвращает байт прав записи параметров, LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getReadWriteFlags();` |
| `bool isNominalVentilationTransferEnabled() const` (public) | Возвращает разрешение передачи номинальной вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isNominalVentilationTransferEnabled();` |
| `bool isNominalVentilationWritable() const` (public) | Возвращает право записи номинальной вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isNominalVentilationWritable();` |
| `VentilationRemoteParameters& setTransferFlags(uint8_t flags)` (public) | Изменяет байт разрешений передачи параметров, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationRemoteParameters& для цепочки; для редактирования локального сообщения/ответа. | `m.setTransferFlags(0);` |
| `VentilationRemoteParameters& setReadWriteFlags(uint8_t flags)` (public) | Изменяет байт прав записи параметров, LB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationRemoteParameters& для цепочки; для редактирования локального сообщения/ответа. | `m.setReadWriteFlags(0);` |
| `VentilationRemoteParameters& setNominalVentilationTransferEnabled(bool value)` (public) | Изменяет разрешение передачи номинальной вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationRemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setNominalVentilationTransferEnabled(true);` |
| `VentilationRemoteParameters& setNominalVentilationWritable(bool value)` (public) | Изменяет право записи номинальной вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationRemoteParameters& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setNominalVentilationWritable(true);` |
| `VentilationRemoteParameters reply(uint8_t transferFlags, uint8_t readWriteFlags) const` (public) | Тип MT::READ_ACK. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0, 0);` |
| `VentilationRemoteParameters reply( Protocol::MessageType type, uint8_t transferFlags, uint8_t readWriteFlags ) const` (public) | Тип явно передан. Кодирует заданные поля согласно описанным выше расположению/единицам. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0, 0);` |

### Messages::VentilationStatus

ID70: запросы master вентиляции в HB, фактическое состояние в LB. Позиции/смысл флагов отличаются от Status ID0. Для рекуперации, байпаса и свободной вентиляции.

Исходник: [VentilationStatus.h](../src/Messages/VentilationStatus.h).

Наследует публичные методы [Messages::Message](#messagesmessage), описанные выше: raw/metadata/status, build/parse и as<T>(). Правильный формат и ID выбирает приложение.

| Сигнатура и доступ | Назначение, работа и применение | Пример вызова |
| --- | --- | --- |
| `VentilationStatus()` (public) | Создаёт READ_DATA фиксированного ID с нулевым payload, новой NONE metadata. Для подготовки сообщения данного ID; отправка отдельно. | `M::VentilationStatus m{};` |
| `explicit VentilationStatus(const Protocol::MessageData& snapshot)` (public) | Копирует весь снимок, включая raw/ошибочную parity, request и operation metadata; не проверяет ID. Для получения типизированного представления результата. | `M::VentilationStatus m{snapshot};` |
| `explicit VentilationStatus(const Message& message)` (public) | Копирует getMessageData() базового сообщения; metadata и raw сохранены, формат/ID не проверяются. Используется также as<T>(). | `M::VentilationStatus m{result};` |
| `explicit VentilationStatus(uint8_t masterFlags, uint8_t lowByte = 0)` (public) | Создаёт новый кадр из полей masterFlags, lowByte; расположение/единицы описаны выше, default type указан в сигнатуре. Metadata новая, маршрута принятого запроса нет; для сообщения с заданными полями. | `M::VentilationStatus m{0, 0};` |
| `uint8_t getMasterFlags() const` (public) | Возвращает байт запрашиваемых флагов master, HB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getMasterFlags();` |
| `uint8_t getSlaveFlags() const` (public) | Возвращает байт фактического состояния slave, LB; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. | `auto value = m.getSlaveFlags();` |
| `bool isVentilationEnabled() const` (public) | Возвращает запрос разрешения вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0100. | `auto value = m.isVentilationEnabled();` |
| `bool isBypassOpenRequested() const` (public) | Возвращает запрос открытия байпаса в ручном режиме; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0200. | `auto value = m.isBypassOpenRequested();` |
| `bool isAutomaticBypassEnabled() const` (public) | Возвращает запрос разрешения автоматического байпаса; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0400. | `auto value = m.isAutomaticBypassEnabled();` |
| `bool isFreeVentilationEnabled() const` (public) | Возвращает запрос разрешения свободной вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0800. | `auto value = m.isFreeVentilationEnabled();` |
| `bool isFault() const` (public) | Возвращает признак неисправности slave; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0001. | `auto value = m.isFault();` |
| `bool isVentilationActive() const` (public) | Возвращает фактическая активность вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0002. | `auto value = m.isVentilationActive();` |
| `bool isBypassOpen() const` (public) | Возвращает фактическое открытие байпаса; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0004. | `auto value = m.isBypassOpen();` |
| `bool isAutomaticBypassActive() const` (public) | Возвращает фактическая активность автоматического байпаса; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0008. | `auto value = m.isAutomaticBypassActive();` |
| `bool isFreeVentilationActive() const` (public) | Возвращает фактическая активность свободной вентиляции; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0010. | `auto value = m.isFreeVentilationActive();` |
| `bool isDiagnostic() const` (public) | Возвращает признак диагностического события; только чтение payload, без проверки ID/успешности. Для декодирования/отображения этого поля. Payload mask=0x0040. | `auto value = m.isDiagnostic();` |
| `VentilationStatus& setMasterFlags(uint8_t flags)` (public) | Изменяет байт запрашиваемых флагов master, HB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. | `m.setMasterFlags(0);` |
| `VentilationStatus& setSlaveFlags(uint8_t flags)` (public) | Изменяет байт фактического состояния slave, LB; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. | `m.setSlaveFlags(0);` |
| `VentilationStatus& setVentilationEnabled(bool value)` (public) | Изменяет запрос разрешения вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0100. | `m.setVentilationEnabled(true);` |
| `VentilationStatus& setBypassOpenRequested(bool value)` (public) | Изменяет запрос открытия байпаса в ручном режиме; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0200. | `m.setBypassOpenRequested(true);` |
| `VentilationStatus& setAutomaticBypassEnabled(bool value)` (public) | Изменяет запрос разрешения автоматического байпаса; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0400. | `m.setAutomaticBypassEnabled(true);` |
| `VentilationStatus& setFreeVentilationEnabled(bool value)` (public) | Изменяет запрос разрешения свободной вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0800. | `m.setFreeVentilationEnabled(true);` |
| `VentilationStatus& setFault(bool value)` (public) | Изменяет признак неисправности slave; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0001. | `m.setFault(true);` |
| `VentilationStatus& setVentilationActive(bool value)` (public) | Изменяет фактическая активность вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0002. | `m.setVentilationActive(true);` |
| `VentilationStatus& setBypassOpen(bool value)` (public) | Изменяет фактическое открытие байпаса; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0004. | `m.setBypassOpen(true);` |
| `VentilationStatus& setAutomaticBypassActive(bool value)` (public) | Изменяет фактическая активность автоматического байпаса; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0008. | `m.setAutomaticBypassActive(true);` |
| `VentilationStatus& setFreeVentilationActive(bool value)` (public) | Изменяет фактическая активность свободной вентиляции; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0010. | `m.setFreeVentilationActive(true);` |
| `VentilationStatus& setDiagnostic(bool value)` (public) | Изменяет признак диагностического события; value/flags задаёт новое поле, остальные payload bits и metadata принятого запроса сохраняются, parity пересчитывается. Возвращает VentilationStatus& для цепочки; для редактирования локального сообщения/ответа. Payload mask=0x0040. | `m.setDiagnostic(true);` |
| `VentilationStatus reply(uint8_t slaveFlags) const` (public) | Тип MT::READ_ACK. Повторяет весь master HB; заменяет slave LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(0);` |
| `VentilationStatus reply(Protocol::MessageType type, uint8_t slaveFlags) const` (public) | Тип явно передан. Повторяет весь master HB; заменяет slave LB. Возвращает новый wrapper с исходным ID/sequence и request=исходный frame; остальная metadata новая. Для ответа slave/прокси на сохранённый запрос. | `auto value = m.reply(MT::READ_ACK, 0);` |
## Типы протокола и сырые кадры

Заголовки относительно `src/`: `Protocol/Types.h` и `Protocol/Frame.h`; пространство имён — `ESP32OT::Protocol`. Определения доступны через общий `ESP32OT.h`. Кадр данных — беззнаковое 32-битное слово: чётность в бите31, тип в битах30..28, неиспользуемые/резервные биты27..24, ID в битах23..16 и payload в битах15..0. Стартовый/стоповый биты Manchester относятся к транспортному оформлению и в этом слове не хранятся.

### `ESP32OT::Protocol::MessageType`

`enum class : uint8_t` содержит все восемь кодов типа на линии. Один код типа не подтверждает чётность, тайминги и соответствие активному запросу.

Исходник: [Protocol/Types.h](../src/Protocol/Types.h).

| Имя | Значение | Назначение |
| --- | --- | --- |
| `READ_DATA` | 0 | Запрос чтения от master. |
| `WRITE_DATA` | 1 | Запрос записи от master. |
| `INVALID_DATA` | 2 | Тип invalid-data со стороны master; isValidRequest() его не принимает. |
| `RESERVED` | 3 | Резервный тип; не принимается ни как запрос, ни как ответ. |
| `READ_ACK` | 4 | Положительное подтверждение чтения; смысл payload зависит от ID. |
| `WRITE_ACK` | 5 | Положительное подтверждение записи. |
| `DATA_INVALID` | 6 | Валидный отрицательный ответ: запрошенные данные/операция недоступны или невалидны. |
| `UNKNOWN_DATA_ID` | 7 | Валидный отрицательный ответ: ID не поддерживается. |

```cpp
auto type = ESP32OT::MessageType::READ_DATA;
uint32_t raw = ESP32OT::Protocol::Frame::build(type, ESP32OT::MessageID::Tboiler);
```

### `ESP32OT::Protocol::MessageID`

`enum class : uint8_t` присваивает имена известным ID. Это числовая метка, а не схема времени выполнения, проверка прав или обещание поддержки ID устройством. Неизвестный/vendor ID можно задать явным приведением и использовать через raw API.

Исходник: [Protocol/Types.h](../src/Protocol/Types.h).

Таблица содержит каждый элемент из `Types.h`; формат означает кодирование payload, а не преобразование физических единиц. `f8.8` — знаковое фиксированное число, `u8/u8` — HB/LB, `flag8` — восемь флагов. Возможность чтения/записи определяется протоколом и устройством, а не наличием элемента enum.

| ID | Элемент | Формат | Назначение / единица |
| --- | --- | --- | --- |
| 0 | `Status` | `flag8/flag8` | Флаги запрошенного состояния master и фактического состояния slave. |
| 1 | `TSet` | `f8.8` | Заданная температура подачи CH, °C. |
| 2 | `MConfigMMemberIDcode` | `flag8/u8` | Флаги конфигурации и идентификатор участника master. |
| 3 | `SConfigSMemberIDcode` | `flag8/u8` | Флаги конфигурации и идентификатор участника slave. |
| 4 | `RemoteRequest` | `u8/u8` | Удалённая команда и её код ответа. |
| 5 | `ASFflags` | `flag8/u8` | Флаги неисправности и OEM-код. |
| 6 | `RBPflags` | `flag8/flag8` | Разрешения передачи и записи удалённых параметров. |
| 7 | `CoolingControl` | `f8.8` | Сигнал управления охлаждением, %.  |
| 8 | `TsetCH2` | `f8.8` | Заданная температура подачи CH2, °C. |
| 9 | `TrOverride` | `f8.8` | Удалённая уставка температуры помещения. |
| 10 | `TSP` | `u8/u8` | Количество прозрачных параметров slave (TSP). |
| 11 | `TSPindexTSPvalue` | `u8/u8` | Индекс и значение TSP. |
| 12 | `FHBsize` | `u8/u8` | Число записей истории ошибок FHB. |
| 13 | `FHBindexFHBvalue` | `u8/u8` | Индекс и значение записи FHB. |
| 14 | `MaxRelModLevelSetting` | `f8.8` | Ограничение относительной модуляции, %.  |
| 15 | `MaxCapacityMinModLevel` | `u8/u8` | Максимальная мощность, кВт / минимальная модуляция, %.  |
| 16 | `TrSet` | `f8.8` | Уставка температуры помещения, °C. |
| 17 | `RelModLevel` | `f8.8` | Относительная модуляция, %.  |
| 18 | `CHPressure` | `f8.8` | Давление воды CH, бар. |
| 19 | `DHWFlowRate` | `f8.8` | Расход воды DHW, л/мин. |
| 20 | `DayTime` | `special/u8` | День недели, часы и минуты. |
| 21 | `Date` | `u8/u8` | Месяц и день месяца. |
| 22 | `Year` | `u16` | Год. |
| 23 | `TrSetCH2` | `f8.8` | Уставка температуры помещения CH2, °C. |
| 24 | `Tr` | `f8.8` | Температура помещения, °C. |
| 25 | `Tboiler` | `f8.8` | Температура воды подачи котла, °C. |
| 26 | `Tdhw` | `f8.8` | Температура DHW, °C. |
| 27 | `Toutside` | `f8.8` | Наружная температура, °C. |
| 28 | `Tret` | `f8.8` | Температура обратной воды, °C. |
| 29 | `Tstorage` | `f8.8` | Температура солнечного накопителя, °C. |
| 30 | `Tcollector` | `f8.8` | Температура солнечного коллектора, °C; библиотека сохраняет f8.8. |
| 31 | `TflowCH2` | `f8.8` | Температура подачи CH2, °C. |
| 32 | `Tdhw2` | `f8.8` | Вторая температура DHW, °C. |
| 33 | `Texhaust` | `s16` | Температура дымовых газов, знаковое целое °C. |
| 34 | `TboilerHeatExchanger` | `f8.8` | Температура теплообменника, °C. |
| 35 | `BoilerFanSpeedSetpointAndActual` | `u8/u8` | Заданная и фактическая скорость вентилятора котла. |
| 36 | `FlameCurrent` | `f8.8` | Ток пламени, мкА. |
| 37 | `TrCH2` | `f8.8` | Температура помещения CH2, °C. |
| 38 | `RelativeHumidity` | `f8.8` | Относительная влажность, %.  |
| 39 | `TrOverride2` | `f8.8` | Вторая удалённая уставка температуры помещения. |
| 48 | `TdhwSetUBTdhwSetLB` | `s8/s8` | Верхняя и нижняя границы уставки DHW, знаковые °C. |
| 49 | `MaxTSetUBMaxTSetLB` | `s8/s8` | Верхняя и нижняя границы максимальной уставки CH, знаковые °C. |
| 56 | `TdhwSet` | `f8.8` | Удалённая уставка DHW, °C. |
| 57 | `MaxTSet` | `f8.8` | Удалённая максимальная уставка CH, °C. |
| 70 | `StatusVentilationHeatRecovery` | `flag8/flag8` | Флаги master/slave вентиляции/рекуперации. |
| 71 | `Vset` | `-/u8` | Относительное задание вентиляции, 0–100%. |
| 72 | `ASFflagsOEMfaultCodeVentilationHeatRecovery` | `flag8/u8` | Флаги неисправностей и OEM-код вентиляции. |
| 73 | `OEMDiagnosticCodeVentilationHeatRecovery` | `u16` | OEM-диагностический код вентиляции. |
| 74 | `SConfigSMemberIDCodeVentilationHeatRecovery` | `flag8/u8` | Конфигурация и member ID вентиляции. |
| 75 | `OpenThermVersionVentilationHeatRecovery` | `f8.8` | Версия протокола вентиляции. |
| 76 | `VentilationHeatRecoveryVersion` | `u8/u8` | Версия и тип изделия вентиляции. |
| 77 | `RelVentLevel` | `-/u8` | Фактическая относительная вентиляция, %.  |
| 78 | `RHexhaust` | `-/u8` | Влажность вытяжного воздуха, %.  |
| 79 | `CO2exhaust` | `u16` | CO2 вытяжного воздуха, ppm. |
| 80 | `Tsi` | `f8.8` | Температура притока на входе, °C. |
| 81 | `Tso` | `f8.8` | Температура притока на выходе, °C. |
| 82 | `Tei` | `f8.8` | Температура вытяжки на входе, °C. |
| 83 | `Teo` | `f8.8` | Температура вытяжки на выходе, °C. |
| 84 | `RPMexhaust` | `u16` | Обороты вытяжного вентилятора, об/мин. |
| 85 | `RPMsupply` | `u16` | Обороты приточного вентилятора, об/мин. |
| 86 | `RBPflagsVentilationHeatRecovery` | `flag8/flag8` | Разрешения удалённых параметров вентиляции. |
| 87 | `NominalVentilationValue` | `u8/-` | Номинальное относительное значение вентиляции, %.  |
| 88 | `TSPventilationHeatRecovery` | `u8/u8` | Количество TSP вентиляции. |
| 89 | `TSPindexTSPvalueVentilationHeatRecovery` | `u8/u8` | Индекс и значение TSP вентиляции. |
| 90 | `FHBsizeVentilationHeatRecovery` | `u8/u8` | Размер FHB вентиляции. |
| 91 | `FHBindexFHBvalueVentilationHeatRecovery` | `u8/u8` | Индекс и значение FHB вентиляции. |
| 93 | `Brand` | `u8/u8` | Индекс и символ строки бренда. |
| 94 | `BrandVersion` | `u8/u8` | Индекс и символ строки версии бренда. |
| 95 | `BrandSerialNumber` | `u8/u8` | Индекс и символ серийного номера. |
| 96 | `CoolingOperationHours` | `u16` | Часы работы охлаждения. |
| 97 | `PowerCycles` | `u16` | Количество включений питания. |
| 98 | `RFsensorStatusInformation` | `special/special` | Индекс RF-датчика, его сигнал и батарея. |
| 99 | `RemoteOverrideOperatingModeHeatingDHW` | `special/special` | Удалённые режимы HC1, HC2 и DHW. |
| 100 | `RemoteOverrideFunction` | `flag8/-` | Флаги приоритета ручного/программного изменения; wrapper использует LB. |
| 101 | `StatusSolarStorage` | `flag8/flag8` | Флаги master/slave солнечного накопителя. |
| 102 | `ASFflagsOEMfaultCodeSolarStorage` | `flag8/u8` | Флаги неисправностей и OEM-код солнечного накопителя. |
| 103 | `SConfigSMemberIDcodeSolarStorage` | `flag8/u8` | Конфигурация и member ID солнечного накопителя; положение system-type неоднозначно. |
| 104 | `SolarStorageVersion` | `u8/u8` | Версия и тип солнечного накопителя. |
| 105 | `TSPSolarStorage` | `u8/u8` | Количество TSP солнечного накопителя. |
| 106 | `TSPindexTSPvalueSolarStorage` | `u8/u8` | Индекс и значение TSP солнечного накопителя. |
| 107 | `FHBsizeSolarStorage` | `u8/u8` | Размер FHB солнечного накопителя. |
| 108 | `FHBindexFHBvalueSolarStorage` | `u8/u8` | Индекс и значение FHB солнечного накопителя. |
| 109 | `ElectricityProducerStarts` | `U16` | Количество запусков электрогенератора. |
| 110 | `ElectricityProducerHours` | `U16` | Часы работы электрогенератора. |
| 111 | `ElectricityProduction` | `U16` | Текущая электрическая мощность, Вт. |
| 112 | `CumulativElectricityProduction` | `U16` | Накопленная электрическая энергия, кВт·ч. |
| 113 | `UnsuccessfulBurnerStarts` | `u16` | Количество неуспешных запусков горелки. |
| 114 | `FlameSignalTooLowNumber` | `u16` | Количество слишком слабых сигналов пламени. |
| 115 | `OEMDiagnosticCode` | `u16` | OEM-диагностический/сервисный код. |
| 116 | `SuccessfulBurnerStarts` | `u16` | Количество успешных запусков горелки. |
| 117 | `CHPumpStarts` | `u16` | Количество запусков насоса CH. |
| 118 | `DHWPumpValveStarts` | `u16` | Количество запусков насоса/клапана DHW. |
| 119 | `DHWBurnerStarts` | `u16` | Количество запусков горелки DHW. |
| 120 | `BurnerOperationHours` | `u16` | Часы горения. |
| 121 | `CHPumpOperationHours` | `u16` | Часы работы насоса CH. |
| 122 | `DHWPumpValveOperationHours` | `u16` | Часы работы насоса/клапана DHW. |
| 123 | `DHWBurnerOperationHours` | `u16` | Часы горения в DHW. |
| 124 | `OpenThermVersionMaster` | `f8.8` | Версия протокола master. |
| 125 | `OpenThermVersionSlave` | `f8.8` | Версия протокола slave. |
| 126 | `MasterVersion` | `u8/u8` | Версия и тип изделия master. |
| 127 | `SlaveVersion` | `u8/u8` | Версия и тип изделия slave. |

```cpp
auto request = ESP32OT::Messages::Message::read(ESP32OT::MessageID::Tboiler);
auto vendor = static_cast<ESP32OT::MessageID>(200);
```

### `ESP32OT::Protocol::OperationStatus`

`enum class : uint8_t` описывает видимый приложению ход и результат операции независимо от наличия принятого кадра. Значения0..9 сохранены; статусы валидных отрицательных ответов добавлены как10 и11.

Исходник: [Protocol/Types.h](../src/Protocol/Types.h).

| Имя | Значение | Смысл |
| --- | --- | --- |
| `NONE` | 0 | Операции/результата пока нет. |
| `QUEUED` | 1 | Принято локально; ожидание worker/готовности. |
| `TRANSMITTING` | 2 | Worker начинает/выполняет передачу. |
| `WAITING` | 3 | Передача master закончилась; ожидается ответ. |
| `REQUEST_RECEIVED` | 4 | Новый валидный запрос на slave с последовательностью маршрутизации. |
| `SUCCESS` | 5 | Положительный ответ master либо завершённая передача slave. |
| `INVALID` | 6 | Неверные тайминги/чётность/тип либо чужой ID ответа активному master. |
| `TIMEOUT` | 7 | Бюджет ожидания приёма истёк без завершённого ответа. |
| `ERROR` | 8 | Ошибка драйвера/ресурсов/жизненного цикла/аргументов; см. esp_err_t. |
| `CANCELLED` | 9 | Операция отменена остановкой либо новым запросом на slave. |
| `DATA_INVALID` | 10 | Валидный отрицательный ответ; терминальный, но не isSuccess(). |
| `UNKNOWN_DATA_ID` | 11 | Валидный ответ о неподдерживаемом ID; терминальный, но не isSuccess(). |

```cpp
if (result.getStatus() == ESP32OT::OperationStatus::UNKNOWN_DATA_ID) {
  Serial.println("ID is not supported");
}
```

### `ESP32OT::MessageID / MessageType / OperationStatus`

Псевдонимы трёх Protocol enum в корневом пространстве имён; дублирующихся значений и затрат преобразования нет. Короткие имена удобны в прикладном коде.

Исходник: [Protocol/Types.h](../src/Protocol/Types.h).

```cpp
ESP32OT::OperationStatus status = ESP32OT::Protocol::OperationStatus::SUCCESS;
```

### `ESP32OT::Protocol::MessageData`

Обычный снимок по значению между фасадом, сообщениями и транспортом. Не владеет динамической памятью, явных методов нет. Кадр сохраняется и при INVALID; hasFrame и status — разные признаки. Ниже указаны исходные значения полей.

Исходник: [Protocol/Types.h](../src/Protocol/Types.h).

| Поле/исходное значение | Назначение |
| --- | --- |
| `uint32_t frame = 0` | Текущее сырое32-битное слово; принятые биты сохраняются. |
| `uint32_t request = 0` | Сырой запрос, связанный с операцией/ответом. |
| `uint32_t operation = 0` | Ненулевой локальный ID исходящей операции;0 для незапрошенного приёма. |
| `uint32_t sequence = 0` | Маршрутная последовательность запроса slave; защищает от устаревших ответов. |
| `uint32_t elapsedUs = 0` | Микросекунды с принятия этой попытки, включая локальное ожидание; это не чистая задержка ответа на линии. |
| `uint32_t totalElapsedUs = 0` | Общее время операции фасада по синхронным попыткам; в транспорте равно времени попытки. |
| `esp_err_t error = ESP_OK` | Ошибка SDK/драйвера; при протокольно невалидном кадре может быть ESP_OK. |
| `OperationStatus status = NONE` | Классификация хода/результата. |
| `uint8_t attempt = 0` | Номер принятой попытки с1; до попыток0. |
| `bool hasFrame = false` | Есть полный принятый кадр; это не утверждение валидности или успешности данных. |

```cpp
ESP32OT::Protocol::MessageData snapshot{};
snapshot.frame = ESP32OT::Protocol::Frame::build(ESP32OT::MessageType::READ_ACK, ESP32OT::MessageID::Tboiler, 0x3200);
snapshot.hasFrame = true;
snapshot.status = ESP32OT::OperationStatus::SUCCESS;
ESP32OT::Messages::Message received(snapshot);
```

### `ESP32OT::Protocol::Frame`

Статические функции без состояния для создания и разбора сырых слов; полезны для vendor ID, прозрачной пересылки, тестов и логирования. Они не передают, не ожидают, не разбирают Manchester-тайминги и не проверяют подключение. Создавать объект Frame не требуется.

Исходник: [Protocol/Frame.h](../src/Protocol/Frame.h).

#### `static uint32_t build(MessageType type, MessageID id, uint16_t data = 0)`

Упаковывает тип, ID и payload; при необходимости устанавливает бит31 для чётной чётности. Возвращает новое сырое слово. Применяется для новых кадров; при raw-пересылке следует сохранить принятые биты, а не пересобирать чётность.

```cpp
auto raw = ESP32OT::Protocol::Frame::build(ESP32OT::MessageType::READ_DATA, ESP32OT::MessageID::Tboiler);
```

#### `static bool parity(uint32_t frame)`

Возвращает true при нечётном числе единиц во всех32 битах. Для кадра с правильной чётной чётностью возвращает false. Полезна в диагностике; слово не изменяет.

```cpp
bool parityError = ESP32OT::Protocol::Frame::parity(raw);
```

#### `static bool isValidRequest(uint32_t frame)`

Возвращает true только при чётной чётности и READ_DATA/WRITE_DATA. Не проверяет доступность ID, резервные биты, ограничения payload и тайминги.

```cpp
bool requestFormatOK = ESP32OT::Protocol::Frame::isValidRequest(raw);
```

#### `static bool isValidResponse(uint32_t frame)`

Возвращает true при чётной чётности и READ_ACK, WRITE_ACK, DATA_INVALID либо UNKNOWN_DATA_ID. Не доказывает успех данных, совпадение ID активного запроса или парность типа запроса и ACK.

```cpp
bool replyFormatOK = ESP32OT::Protocol::Frame::isValidResponse(raw);
```

#### `static MessageType getType(uint32_t frame)`

Извлекает биты30..28 без проверки; можно применять к невалидному сырому кадру.

```cpp
auto type = ESP32OT::Protocol::Frame::getType(raw);
```

#### `static MessageID getId(uint32_t frame)`

Извлекает биты23..16 как MessageID; неизвестные числовые ID сохраняются.

```cpp
auto id = ESP32OT::Protocol::Frame::getId(raw);
```

#### `static uint16_t getData(uint32_t frame)`

Возвращает младшие16 бит без изменений; смысл определяет выбранный wrapper или приложение.

```cpp
uint16_t payload = ESP32OT::Protocol::Frame::getData(raw);
```

#### `static const char* typeToString(MessageType type)`

Возвращает статическое имя в верхнем регистре для0..7, иначе "UNKNOWN". Без выделения памяти; строка принадлежит библиотеке.

```cpp
Serial.println(ESP32OT::Protocol::Frame::typeToString(type));
```

#### `static const char* statusToString(OperationStatus status)`

Возвращает статическое имя в верхнем регистре для0..11, иначе "UNKNOWN". Применяется в логах; без выделения памяти.

```cpp
Serial.println(ESP32OT::Protocol::Frame::statusToString(result.getStatus()));
```

## Конфигурация и callbacks транспорта

Обычная точка входа — фасад. Объявления транспорта доступны для интеграции и сопровождения, но фасад владеет одним RmtTransport и его worker. RmtTransport владеет RmtDriver; приложение не должно одновременно обслуживать ручной цикл driver.poll().

### `ESP32OT::Transport::Config`

Конфигурация по значению для запуска worker; фасад предоставляет её псевдонимом ESP32OT::ESP32OT::Config. Явных методов нет. Изменение конфигурации остановленного фасада заменяет всё значение; поля транспорта не служат регулировками на ходу.

Исходник: [Transport/RmtTransport.h](../src/Transport/RmtTransport.h).

| Поле/исходное значение | Назначение и ограничения |
| --- | --- |
| `gpio_num_t rxGpio = GPIO_NUM_NC` | Входной GPIO адаптера; должен быть допустимым GPIO. |
| `gpio_num_t txGpio = GPIO_NUM_NC` | Выходной GPIO адаптера; должен поддерживать вывод. |
| `bool isSlave = false` | False: ESP32 master к котлу; true: slave к термостату. |
| `bool alwaysReceive = false` | Принимать незапрошенные кадры и в READY; активный запрос по-прежнему проверяет ID. |
| `uint8_t maxAttempts = 5` | Всего синхронных попыток master, включая первую;0 означает1. Используется политикой повторов фасада, а не циклом внутри sendMessage. |
| `uint32_t stackSize = 3072` | Размер стека закреплённого worker в байтах ESP-IDF FreeRTOS; включает callbacks. Не ниже configMINIMAL_STACK_SIZE. |
| `UBaseType_t priority = 5` | Приоритет задачи FreeRTOS; меньше configMAX_PRIORITIES. |
| `BaseType_t core = 0` | Номер ядра [0,portNUM_PROCESSORS); worker должен быть закреплён. |
| `bool acceptInvalidRequests = false` | Разрешение slave/proxy принимать полные невалидные запросы. Сохраняет INVALID и сырые биты; частичные захваты запросами не становятся. |

```cpp
ESP32OT::ESP32OT::Config config;
config.rxGpio = GPIO_NUM_35;
config.txGpio = GPIO_NUM_36;
config.maxAttempts = 1;
controller.setConfig(config);
```

### `ESP32OT::Transport::StatusEvent`

Временное представление callback: предыдущий и текущий статусы и const-ссылка на соответствующий Message. Методов нет. Message прекращает существовать после выхода из callback; при необходимости сохраните копию по значению.

Исходник: [Transport/RmtTransport.h](../src/Transport/RmtTransport.h).

| Поле | Смысл |
| --- | --- |
| `OperationStatus previous` | Статус до перехода. |
| `OperationStatus current` | Статус после перехода. |
| `const Messages::Message& message` | Снимок текущего события; событие статуса может не иметь кадра. |

### `ESP32OT::Transport::StatusCallback / MessageCallback`

Владеющие одноаргументные aliases: std::function<void(const StatusEvent&)> и std::function<void(const Messages::Message&)>; поддерживают захватывающие лямбды. Непустая регистрация выделяет совместное хранилище callable; захваты могут выделять больше. Вызов сохраняет совместное владение вместо копирования callable. Исполняются worker вне ISR/внутренних locks. Message-hooks независимо охватывают RX и pre-TX согласно роли; только RX гарантирует hasFrame. Пустой/null снимает hook. Замена не ждёт завершения вызова; объекты по ссылкам принадлежат вызывающему.

Исходник: [Transport/RmtTransport.h](../src/Transport/RmtTransport.h).

```cpp
const char* label = "bus ";
ESP32OT::Transport::StatusCallback statusHook = [label](const ESP32OT::Transport::StatusEvent& event) {
  Serial.print(label);
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
};
ESP32OT::Transport::MessageCallback messageHook = [label](const ESP32OT::Messages::Message& message) {
  Serial.print(label);
  Serial.println(message.getRaw(), HEX);
};
```

### `ESP32OT::Transport::LegacyStatusCallback / LegacyMessageCallback`

Указатели совместимости: void (*)(const StatusEvent&, void*) и void (*)(const Messages::Message&, void*). Перегрузки адаптируют пару функции/context во владеющий одноаргументный observer. Сам context заимствован, не принадлежит библиотеке. Прежние вызовы регистрации поддерживаются; явно типизированные старые callbacks используют Legacy aliases. nullptr снимает соответствующий hook.

Исходник: [Transport/RmtTransport.h](../src/Transport/RmtTransport.h).

```cpp
static void statusChanged(const ESP32OT::Transport::StatusEvent& event, void*) {
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
}
static void responseReceived(const ESP32OT::Messages::Message& message, void*) {
  Serial.println(message.getRaw(), HEX);
}
ESP32OT::Transport::LegacyStatusCallback statusHook = statusChanged;
ESP32OT::Transport::LegacyMessageCallback responseHook = responseReceived;
```

### `ESP32OT::Transport::RmtTransport`

Транспорт, владеющий worker. Сериализует принятые команды и публикует снимки по значению. Поддерживается ровно одна ожидающая/выполняемая исходящая операция; принятие команды отличается от передачи и успешного ответа. Публичные методы ниже — контракт нижнего уровня; для приложения предпочтительны аналоги фасада. Если не указано иное, вызов идёт из обычной задачи, не ISR.

Исходник: [Transport/RmtTransport.h](../src/Transport/RmtTransport.h).

#### `explicit RmtTransport(bool isSlave = false)`

Создаёт статические mutex/binary/recursive semaphore и задаёт исходную роль; worker и RMT-каналы не запускаются.

```cpp
ESP32OT::Transport::RmtTransport transport(false);
```

#### `~RmtTransport()`

Вызывает end(), освобождает все три владеющих hook-holder, пока объекты синхронизации ещё существуют, затем удаляет semaphore. Захваченные объекты освобождаются до удаления locks, используемых getters транспорта. При неудачной очистке вызывает ESP_ERROR_CHECK с invalid state, чтобы не освободить память живого worker. Явный end() позволяет обработать/повторить очистку до уничтожения.

```cpp
{ ESP32OT::Transport::RmtTransport transport; /* end before leaving scope */ }
```

#### `RmtTransport(const RmtTransport&) = delete`

Копирование запрещено: владение worker, блокировками и RMT нельзя дублировать.

```cpp
// ESP32OT::Transport::RmtTransport copy(transport); // ill-formed
```

#### `RmtTransport& operator=(const RmtTransport&) = delete`

Присваивание копии запрещено из-за владения ресурсами; экземпляры остаются независимыми.

```cpp
// other = transport; // ill-formed
```

#### `bool begin(const Config& config)`

Проверяет GPIO/core/priority/stack, защищает lifecycle/session, создаёт закреплённый worker и ждёт подтверждения старта. True означает успешный запуск драйвера; false сам по себе не доказывает освобождение ресурсов. Повторный запуск, ISR и вызов из этого worker отвергаются. После успешной настройки аппаратуры worker выдерживает TX в idle (уровень 1) в течение 1150 мс перед переходом в READY и подтверждением запуска. Если переданная роль меняется, оба message-hook отсоединяются, status-hook сохраняется.

```cpp
bool started = transport.begin(config);
```

#### `bool end()`

Запрашивает согласованную остановку, будит worker, ждёт подтверждения и возвращает факт освобождения worker. Повтор после полной остановки успешен. ISR и вызов из самого worker запрещены. Ожидающие/активные команды получают CANCELLED. При ошибке очистки worker сохраняется для повторного end().

```cpp
bool stopped = transport.end();
```

#### `bool configureRole(bool isSlave)`

Меняет роль только без worker, под mutex состояния. При смене отсоединяет оба request/response hook; status-hook сохраняется. Прежние замыкания освобождаются после выхода из mutex. Возвращает false при работе либо ISR.

```cpp
transport.end();
bool changed = transport.configureRole(true);
```

#### `bool sendMessage(const Messages::Message& message, bool response, uint8_t attempt = 1, bool synchronous = false)`

Строит/копирует и принимает ровно одну команду; возвращает принятие, не завершение. response=false — запрос master, true — ответ slave. response должен совпасть с ролью. Ответ slave требует текущую ненулевую sequence сохранённого запроса. attempt только записывается в metadata; повторов внутри метода нет. synchronous=true сохраняет отдельный результат для ожидающего фасада. ISR, занятая session, busy/stopping/неинициализированное состояние и устаревшая sequence отвергаются. Callback запроса slave может поставить свой async-ответ в обход внешней session.

```cpp
bool accepted = transport.sendMessage(ESP32OT::Messages::Message::read(ESP32OT::MessageID::Tboiler), false, 1, false);
```

#### `Protocol::MessageData getResult() const`

Возвращает защищённую mutex копию последнего результата/хода, включая возможный незапрошенный приём. Владение живым RX-буфером не передаётся.

```cpp
auto latest = transport.getResult();
```

#### `Protocol::MessageData getSynchronousResult() const`

Возвращает сохранённый снимок последней команды с synchronous=true. Последующий незапрошенный slave-запрос не заменяет завершение; следующая синхронная команда может заменить.

```cpp
auto completion = transport.getSynchronousResult();
```

#### `Protocol::MessageData getRequest() const`

Копирует сохранённый принятый slave-запрос с sequence маршрутизации. При begin очищается, поэтому старый ответ между сессиями не используется. Сам вызов не доказывает наличие текущего запроса.

```cpp
auto lastRequest = transport.getRequest();
```

#### `bool isInitialized() const`

Возвращает сохранённую инициализацию драйвера вне остановки. Отличается от существования worker, готовности и связи с устройством.

```cpp
bool initialized = transport.isInitialized();
```

#### `bool isReady() const`

True только при инициализации, отсутствии остановки/занятой исходящей команды и READY драйвера после паузы шины. Наличие котла не доказывает.

```cpp
if (transport.isReady()) { /* one command may be accepted */ }
```

#### `bool isBusy() const`

Показывает резервирование/ожидание/работу исходящей команды. Завершённый результат может уже быть, пока драйвер выдерживает MWT и isReady() равен false.

```cpp
bool occupied = transport.isBusy();
```

#### `bool isRunning() const`

Показывает наличие worker, в том числе сохранённого после фатальной ошибки запуска/очистки. Отличает отсутствие инициализации от полной остановки.

```cpp
bool workerExists = transport.isRunning();
```

#### `bool isWorkerTask() const`

True только в worker данного экземпляра; false в ISR и других задачах. Используется для запрета блокирующих повторных lifecycle/sync-вызовов.

```cpp
bool sameWorker = transport.isWorkerTask();
```

#### `esp_err_t getLastError() const`

Возвращает последнюю общую SDK-ошибку под mutex. Рассматривается вместе со статусом, поскольку ESP_OK возможен при INVALID и валидном отрицательном ответе.

```cpp
esp_err_t error = transport.getLastError();
```

#### `uint32_t getStackHighWaterMark() const`

Возвращает наблюдавшийся минимум неиспользованного стека worker в байтах этого ESP-IDF port;0 без worker. Это наблюдавшийся остаток, не гарантированный запас.

```cpp
uint32_t unusedBytes = transport.getStackHighWaterMark();
```

#### `bool takeSession()`

Пытается без блокирования взять recursive mutex session вызывающей задачи; false в ISR либо при владении другой задачей. Фасад держит session на весь синхронный вызов/повторы.

```cpp
if (transport.takeSession()) { /* matched releaseSession required */ transport.releaseSession(); }
```

#### `void releaseSession()`

Освобождает одно соответствующее рекурсивное взятие session той же задачей; без успешного takeSession() не вызывается.

```cpp
transport.releaseSession(); // only after this task acquired it
```

#### `void onStatusChange(StatusCallback callback = nullptr)`

Назначает/снимает независимый владеющий std::function status-hook. Уведомления возникают только при смене статуса; Message может не иметь принятого кадра. Непустая регистрация выделяет совместное хранилище до mutex; замена меняет holders и освобождает прежний после unlock. Вызов сохраняет совместное владение, не копируя захваты, и исполняется вне locks/ISR. Захваченные ссылки заимствованы; замена/снятие не ждёт завершения вызова.

```cpp
transport.onStatusChange([](const ESP32OT::Transport::StatusEvent& event) {
  Serial.println(ESP32OT::Protocol::Frame::statusToString(event.current));
});
```

#### `void onStatusChange(LegacyStatusCallback callback, void* context = nullptr)`

Перегрузка совместимости: оборачивает двухаргументную функцию и заимствованный context во владеющий одноаргументный handler. Null снимает регистрацию. Правила worker, времени жизни и замены совпадают с основной перегрузкой.

```cpp
transport.onStatusChange(statusChanged, nullptr);
```

#### `bool onResponse(MessageCallback callback = nullptr)`

Назначает/снимает независимый response-hook и возвращает true в обеих ролях. Master RX включает полные невалидные/отрицательные/незапрошенные кадры; hasFrame=true. Slave pre-TX вызывает hook один раз для каждой попытки с TRANSMITTING и hasFrame=false перед локальной передачей. Последующая отмена/ошибка может предотвратить TX; hook не подтверждает завершение или доставку. Пустой callback снимает регистрацию. Владение, память и замена совпадают с onStatusChange.

```cpp
const char* label = "RX ";
bool installed = transport.onResponse([label](const ESP32OT::Messages::Message& response) {
  Serial.print(label);
  Serial.println(response.getRaw(), HEX);
});
```

#### `bool onResponse(LegacyMessageCallback callback, void* context = nullptr)`

Адаптирует прежнюю двухаргументную response-функцию и заимствованный context; возвращает true в обеих ролях. nullptr снимает response-hook. Семантика RX/pre-TX и время жизни совпадают с основной перегрузкой.

```cpp
bool installed = transport.onResponse(responseReceived, nullptr);
```

#### `bool onRequest(MessageCallback callback = nullptr)`

Назначает/снимает независимый request-hook и возвращает true в обеих ролях. Slave RX передаёт полные запросы, включая невалидные; routing sequence получают только валидные либо явно разрешённые невалидные запросы. Master pre-TX вызывает hook один раз для попытки с TRANSMITTING и hasFrame=false; это не доказательство начала/завершения TX. Пустой callback снимает регистрацию. Владение и замена совпадают с onStatusChange; observer должен быть коротким, поскольку предшествует TX.

```cpp
const char* label = "TX ";
transport.onRequest([label](const ESP32OT::Messages::Message& request) {
  Serial.print(label);
  Serial.println(request.getRaw(), HEX);
});
```

#### `bool onRequest(LegacyMessageCallback callback, void* context = nullptr)`

Адаптирует прежнюю request-функцию с context во владеющий observer. Возвращает true в обеих ролях; nullptr снимает регистрацию. Context заимствован и должен существовать до завершения работающих/подготовленных вызовов.

```cpp
bool installed = transport.onRequest(requestReceived, nullptr);
```

`ESP32OT::ESP32OT::sendMessage` — private-helper фасада, возвращающий завершённый Protocol::MessageData и выполняющий синхронное ожидание/учёт повторов; публичные send-методы фасада оборачивают результат в Message либо выбранный wrapper. `Transport::RmtTransport::sendMessage` выше возвращает bool сразу после принятия одной команды; его используют и async-методы фасада. Совпадение имён не делает поведение взаимозаменяемым.

### Внутреннее устройство RmtTransport

### `ESP32OT::Transport::RmtTransport::ActiveOperation`

Private-структура worker, определённая в CPP. Методов нет. `message` содержит MessageData; `active=false` означает отсутствие команды, `sent=false` — драйвер ещё не получил её, `isResponse=false` выбирает запрос master, `txNotified=false` означает, что исходящий observer ещё не вызван, `acceptedAt=0` — uint64_t время esp_timer в мкс. Worker переносит сюда pendingCommand, не отдавая структуру приложению.

Исходник: [Transport/RmtTransport.cpp](../src/Transport/RmtTransport.cpp).

### Приватные методы RmtTransport

Все методы этой группы имеют доступ C++ private и вызываются только внутри реализации.

#### `static void runTask(void* context)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Точка входа FreeRTOS: приводит context к this, исполняет worker экземпляра и после выхода run() удаляет текущую задачу.

```cpp
// Passed as xTaskCreatePinnedToCore(runTask, ..., this, ...)
```

#### `bool configureRole(bool isSlave, std::shared_ptr<const MessageCallback>& retiredRequest, std::shared_ptr<const MessageCallback>& retiredResponse)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Private-реализация смены остановленной роли. Под mutex отклоняет работающий worker, меняет роль и переносит оба message-hook в переданные retired holders. Фасад сохраняет holders до завершения перенастройки/session, чтобы разрушение захваченных объектов не выполнялось внутри блокировок состояния/конфигурации. Status-hook сохраняется.

```cpp
// Implementation only; retired holders outlive the configuration update:
bool changed = configureRole(isSlave, retiredRequest, retiredResponse);
```

#### `void run()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Инициализирует драйвер на закреплённом worker/ядре, подтверждает старт, обрабатывает события перед новыми командами, публикует результаты, один раз вызывает исходящий request/response-observer до попытки TX, повторно проверяет готовый RX/stop/актуальность slave sequence и запускает driver.send либо откладывает/отменяет TX. Ждёт уведомление/deadline; прикладной process() не нужен.

```cpp
static_cast<RmtTransport*>(context)->run();
```

#### `bool stopWorker(ActiveOperation& operation)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Отменяет активную/ожидающую команды, согласованно останавливает драйвер, публикует состояние и подтверждает остановку. True разрешает выход задачи; false сохраняет worker/ресурсы для следующего end().

```cpp
if (stopWorker(operation)) { return; }
```

#### `void processDriverResult(ActiveOperation& operation, uint32_t& lastResultSequence)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Однократно обрабатывает новую sequence драйвера. Классифицирует результат/отрицательный ответ, проверяет ID только активного master-запроса, сохраняет полные невалидные биты и маршрутизирует завершение, пассивный кадр либо slave-запрос.

```cpp
processDriverResult(operation, lastResultSequence);
```

#### `void processSlaveRequest(Protocol::MessageData& received, ActiveOperation& operation)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Для валидных либо разрешённых полных невалидных кадров назначает ненулевую requestSequence, отменяет устаревшие ответы, сохраняет raw/маршрутизацию; валидный статус становится REQUEST_RECEIVED. Обычный невалидный шум не вытесняет валидный запрос.

```cpp
processSlaveRequest(completed, operation);
```

#### `void takePendingOperation(ActiveOperation& operation)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Под mutex переносит единственную pending-команду в свободную ActiveOperation worker; отмечает active/not-sent и вне lock уведомляет о QUEUED.

```cpp
takePendingOperation(operation);
```

#### `void publish(Protocol::MessageData data, bool terminal, bool incoming = false)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Публикует result/synchronous/request под mutex; terminal освобождает busy. После unlock уведомляет status и направляет полный входящий кадр в requestCallback для slave RX либо responseCallback для master RX. Незапрошенный приём не заменяет зарезервированную исходящую операцию, но доходит до соответствующего RX-hook.

```cpp
publish(completed, true, true);
```

#### `void notifyStatus(Protocol::OperationStatus previous, const Protocol::MessageData& data)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

При смене статуса сохраняет shared status-holder под mutex и вызывает его вне lock с временными Message/StatusEvent. Копия holder сохраняет замыкание без копирования захватов и выделения памяти для вызова. Ошибки могут не иметь кадра. Параллельная замена не разрушает замыкание текущего вызова.

```cpp
notifyStatus(previous, operation.message);
```

#### `void notifyMessage(const Protocol::MessageData& data, bool response)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Выбирает responseCallback при response=true, иначе requestCallback; сохраняет shared holder под mutex и вызывает вне lock с временным Message. Захваты std::function при каждом вызове не копируются. RX передаёт полный принятый кадр; pre-TX передаёт TRANSMITTING/hasFrame=false один раз для попытки. Снимок сохраняет заменённое замыкание до завершения вызова.

```cpp
notifyMessage(operation.message, operation.isResponse);
```

#### `void updateDriverState()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Переносит initialized/readiness/error из принадлежащего worker драйвера под общий mutex; stopping подавляет initialized.

```cpp
updateDriverState();
```

Все private-поля: роль `slave`; память semaphore `mutexStorage`, `lifecycleStorage`, `acknowledgmentStorage`, `sessionStorage` и handles `mutex`, `lifecycle`, `acknowledgment`, `session`; worker/драйвер `worker`, `driver`; GPIO/политика `rxGpio`, `txGpio`, `alwaysReceive`, `acceptInvalidRequests`; флаги `initialized`, `ready`, `stopping`, `busy`, `pending`, `pendingResponse`; снимки `result`, `pendingCommand`, `synchronousResult`, `request`; порядок/маршрутизация `synchronousOperation`, `queuedPrevious`, `nextOperation`, `requestSequence`, микросекундное `acceptedAt`; ошибка `lastError`; владеющие shared holders `statusCallback`, `requestCallback`, `responseCallback`; legacy contexts заимствованы внутри замыканий адаптеров. Снимки и общие флаги защищены mutex; драйвер и ActiveOperation принадлежат worker.

## Справочник внутреннего RMT-драйвера

### `ESP32OT::Transport::RmtDriver`

Аппаратный слой/машина состояний; обычно исключительно принадлежит worker RmtTransport. Публично объявленные методы — внутренний контракт worker→driver, а не предложение добавлять прикладные polling-задачи. При запуске фиксируются задача и ядро владельца; аппаратные вызовы требуют того же закреплённого владельца. Явного деструктора нет: транспорт-владелец обязан вызывать end(). Копирование запрещено.

Исходник: [Transport/RmtDriver.h](../src/Transport/RmtDriver.h).

### `ESP32OT::Transport::RmtDriver::State`

`enum class : uint8_t` отслеживает аппаратную активность, ход разбора и обязательную паузу шины. Числа идут по порядку объявления0..8.

Исходник: [Transport/RmtDriver.h](../src/Transport/RmtDriver.h).

| Имя | Значение | Смысл |
| --- | --- | --- |
| `NOT_INITIALIZED` | 0 | Драйвер неактивен либо фатальная ошибка; ресурсы ещё могут существовать. |
| `READY` | 1 | Можно начинать передачу; slave/passive RX может быть включён. |
| `DELAY` | 2 | MWT после терминального захвата перед READY. |
| `REQUEST_SENDING` | 3 | Выполняется RMT TX; действует watchdog передачи. |
| `RESPONSE_WAITING` | 4 | Master ждёт ответ/начало захвата. |
| `RESPONSE_START_BIT` | 5 | Частично разобран стартовый бит. |
| `RESPONSE_RECEIVING` | 6 | Частично разобраны данные. |
| `RESPONSE_READY` | 7 | Разобраны полные32 бита и стоповый переход; poll() ещё проверяет чётность/тип. |
| `RESPONSE_INVALID` | 8 | Захват не дал допустимый разбор полного кадра. |

### `ESP32OT::Transport::RmtDriver::ResultStatus`

`enum class : uint8_t`: NONE0, SUCCESS1, INVALID2, TIMEOUT3. NONE также сопровождает TX/ресурсные ошибки, которые транспорт отображает в ERROR; валидные отрицательные типы сначала получают driver SUCCESS и отдельно классифицируются транспортом.

Исходник: [Transport/RmtDriver.h](../src/Transport/RmtDriver.h).

```cpp
// Context: transport worker implementation, not application loop().
if (driver.getState() == RmtDriver::State::READY) { /* bus may transmit */ }
if (driver.getResultStatus() == RmtDriver::ResultStatus::TIMEOUT) { /* publish */ }
```

### Публичные методы нижнего уровня RmtDriver

Все методы этой группы имеют доступ C++ public. Это контракт worker-владельца с драйвером. Приложения обычно используют фасад.

#### `RmtDriver() = default`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Инициализирует только поля; ресурсов и worker нет.

```cpp
RmtDriver driver;
```

#### `RmtDriver(const RmtDriver&) = delete`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Копирование владения драйвером запрещено.

```cpp
// RmtDriver copy(driver); // ill-formed
```

#### `RmtDriver& operator=(const RmtDriver&) = delete`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Присваивание копии запрещено.

```cpp
// other = driver; // ill-formed
```

#### `bool begin(gpio_num_t rxGpio, gpio_num_t txGpio, bool isSlave, bool alwaysReceive = false)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

На закреплённой задаче владельца проверяет GPIO и остановку/отсутствие ресурсов, выделяет внутренний RmtContext, статические очереди, RX/TX и copy encoder, задаёт 1 МГц и фильтр idle, выдерживает TX в idle (уровень 1) в течение 1150 мс, затем переходит в READY и включает slave/passive RX. При ошибке false/getLastError; очистка может потребовать позднего end().

```cpp
bool started = driver.begin(rxGpio, txGpio, false, false);
```

#### `bool end()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

На owner отключает/удаляет TX перед RX (TX_DONE может включить RX), удаляет encoder/очереди, восстанавливает настроенный GPIO-выход, уничтожает context и сбрасывает разбор/результат. Ошибка сохраняет владение/ресурсы для повтора. Без context повтор безопасен.

```cpp
bool stopped = driver.end();
```

#### `bool send(uint32_t frame)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Только owner и READY: останавливает захват, очищает события/владение буферами, сбрасывает parser/result, сохраняет точное сырое передаваемое слово и запускает sendFrame. Нет исправления чётности и повторов. False означает ошибку/отказ состояния.

```cpp
bool sent = driver.send(operation.message.frame);
```

#### `void poll()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Продвижение worker: сначала готовые события, затем TX-watchdog; готовый захват обрабатывается перед RX-timeout, публикуются full/invalid/timeout, выдерживается MWT master/slave и включается допустимый приём. Управляет таймингами; приложение фасада его не вызывает.

```cpp
driver.poll(); // inside the one owning worker
```

#### `TickType_t waitTicks() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Вычисляет ожидание FreeRTOS по очереди событий и TX/RX/deadline, используя uint32_t разность с переполнением и округление тиков вверх. Возвращает0 при готовой работе/deadline либо portMAX_DELAY без плановой работы. Также возвращает 0, если в READY для slave/passive RX нет активного буфера приёма: poll() возобновляет приём перед ожиданием worker после терминального таймаута или ошибки.

```cpp
ulTaskNotifyTake(pdTRUE, driver.waitTicks());
```

#### `State getState() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает внутреннее состояние без lock; чтение принадлежит worker-владельцу.

```cpp
auto state = driver.getState();
```

#### `bool isReady() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

True только в READY; подключение котла не проверяет.

```cpp
if (driver.isReady()) { /* send may proceed */ }
```

#### `bool isInitialized() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

True при состоянии, отличном от NOT_INITIALIZED; false не означает hasResources()==false.

```cpp
bool initialized = driver.isInitialized();
```

#### `bool hasResources() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Показывает существование RmtContext, включая сохранённые ресурсы после ошибки старта/очистки.

```cpp
if (!driver.isInitialized() && driver.hasResources()) { /* end required */ }
```

#### `uint32_t getFrame() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает сохранённое сырое принятое слово либо переданное slave-слово при завершении TX. Отдельно проверяются hasFrame/resultIsTransmit/status.

```cpp
uint32_t raw = driver.getFrame();
```

#### `ResultStatus getResultStatus() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает последнюю классификацию драйвера; новизна определяется getResultSequence.

```cpp
auto status = driver.getResultStatus();
```

#### `bool hasFrame() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

True только для полного принятого кадра, включая невалидную чётность/тип/тайминги; успешный slave TX не является принятым кадром.

```cpp
bool fullReceive = driver.hasFrame();
```

#### `bool resultIsTransmit() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Отличает завершение slave TX от входящего запроса; оба используют result sequence.

```cpp
bool transmit = driver.resultIsTransmit();
```

#### `uint32_t getTimestamp() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает младшие32 бита esp_timer времени последнего TX_DONE/захвата, в мкс с переполнением. Это timestamp ISR-события, а не elapsedUs фасада.

```cpp
uint32_t eventUs = driver.getTimestamp();
```

#### `uint32_t getResultSequence() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает увеличиваемый с переполнением счётчик результатов; worker сравнивает неравенство для однократной обработки нового результата.

```cpp
uint32_t sequence = driver.getResultSequence();
```

#### `esp_err_t getLastError() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Возвращает сохранённую SDK-ошибку независимо от протокольного статуса.

```cpp
esp_err_t error = driver.getLastError();
```

### Приватные методы RmtDriver

Все методы этой группы имеют доступ C++ private, включая ISR-callbacks, регистрируемые внутри реализации в SDK.

#### `static bool IRAM_ATTR onTransmitDone(rmt_channel_handle_t, const rmt_tx_done_event_data_t*, void*)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

RMT ISR callback: фиксирует TX_DONE, немедленно берёт/включает RX-буфер до пробуждения задачи, ставит событие и возвращает необходимость переключения. Использует только context и ISR-safe очереди.

```cpp
txCallbacks.on_trans_done = onTransmitDone;
```

#### `static bool IRAM_ATTR onReceiveDone(rmt_channel_handle_t, const rmt_rx_done_event_data_t*, void*)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

RMT ISR callback: ставит count/номер буфера/timestamp захвата, передаёт владение, включает альтернативный свободный буфер и уведомляет owner. Сообщения не разбирает, прикладные hooks не вызывает.

```cpp
rxCallbacks.on_recv_done = onReceiveDone;
```

#### `bool isOwnerTask() const`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Требует живой context, отсутствие ISR, совпадение текущей задачи и ядра с владельцем запуска.

```cpp
if (!isOwnerTask()) { return false; }
```

#### `bool startReceive()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

При отсутствии активного буфера берёт свободный индекс и вызывает rmt_receive в фиксированный буфер; при ошибке возвращает владение. True при уже работающем или успешно включённом приёме.

```cpp
if (!startReceive()) { /* publish driver error */ }
```

#### `bool stopReceive()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

При активности отключает RX, возвращает активный буфер и включает канал снова. Без активного захвата успешный no-op. SDK-ошибка даёт false и сохраняет состояние для очистки.

```cpp
if (!stopReceive()) { /* fatal transport failure */ }
```

#### `bool resetTransmitter()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Отключает/включает TX на границе watchdog для безопасной обработки позднего TX_DONE на owner-ядре. Обновляет enabled-флаги и SDK-ошибку.

```cpp
if (!resetTransmitter()) { /* retain resources for end */ }
```

#### `bool sendFrame(uint32_t frame)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Кодирует start1+32 сырых MSB-first бита+stop1 в34 фиксированных RMT-символа, каждый по две половины500мкс, затем неблокирующий TX с idle/eot1. Чётность сохраняется, transmitTimestamp фиксируется до подачи.

```cpp
bool submitted = sendFrame(request);
```

#### `void processRmtEvents()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Обрабатывает готовые TX/RX-события, разбирает допустимые фиксированные захваты, сохраняет timestamp/владение буферами, останавливает терминальный RX и передаёт ресурсные ошибки. Без прикладных callbacks и выделения памяти.

```cpp
processRmtEvents();
```

#### `void publishResult()`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Увеличивает resultSequence, чтобы транспорт увидел новый результат однократно; прикладных callbacks здесь нет.

```cpp
responseStatus = ResultStatus::TIMEOUT;
publishResult();
```

Все private-поля драйвера: указатель `rmt`; GPIO `rxGpio`, `txGpio`; роли `isSlave`, `alwaysReceive`; машина `state`; сырые слова `response`, `transmittedFrame`; результат `responseStatus`, `receivedFrame`, `transmitResult`, `resultSequence`, `lastError`; младшие32 бита времени в мкс `responseTimestamp`, `transmitTimestamp`; parser `responseBitIndex`, `responseTimingValid`. Доступ принадлежит owner-задаче, кроме отдельно управляемых полей RmtContext, передаваемых через очереди.

### Типы, константы и функции декодирования внутри CPP

Следующие символы — детали реализации RmtDriver.cpp, недоступные прикладным translation units. Примеры являются контекстным псевдокодом. Они включены для полноты типов/функций библиотеки; приложения используют Message wrappers и фасад.

| Константа | Значение | Смысл |
| --- | --- | --- |
| `TX_SYMBOL_COUNT` | 34 | RMT-символы: старт+32 данных+стоп. |
| `RX_SYMBOL_COUNT` | 64 | Символов в одном фиксированном RX-буфере; буферов два. |
| `NO_RX_BUFFER` | 2 | Маркер вне допустимых индексов0 и1. |
| `RX_IDLE_US` | 2500 | Порог неактивности захвата, мкс. |
| `RESPONSE_TIMEOUT_US` | 1000000 | Базовый бюджет неактивности RX / TX-watchdog,1с. |
| `RESPONSE_FRAME_GRACE_US` | 40000 | Добавка на завершение RX-кадра,40мс. |
| `RX_MIN_BIT_PERIOD_US` | 900 | Нижний период средних переходов data bit, мкс. |
| `RX_MAX_BIT_PERIOD_US` | 1150 | Верхний период средних переходов data bit, мкс. |

Deadline неактивности RX равен1с+40мс+2,5мс. MWT —100мс master /20мс slave от timestamp RX_DONE. Константы входят в текущий контракт совместимости; период опроса остаётся политикой приложения. Готовый захват обрабатывается раньше решения о timeout. Округление тиков и ISR/task-задержки отличаются от заданных длительностей.

### `ESP32OT::Transport::RmtEventType`

Локальный enum:uint8_t: TRANSMIT_DONE=0, RECEIVE_DONE=1. Помечает события очереди; методов нет.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

### `ESP32OT::Transport::RmtEvent`

Значение очереди, методов нет: RmtEventType type; uint32_t timestamp (младшие32 бита esp_timer,мкс); size_t symbolCount (число RMT-символов RX,0 для TX); uint8_t buffer (0/1 либо NO_RX_BUFFER); esp_err_t error. Передаются индексы буферов, а не живое parser-состояние.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

### `ESP32OT::Transport::RmtDriver::RmtContext`

Private context во внутренней RAM, явных методов нет. Handles rx/tx/encoder; receiveConfig; txSymbols[34]; rxSymbols[2][64]; память очереди событий queueState/queueStorage[3*sizeof(RmtEvent)]/events; очередь свободных буферов availableState/availableStorage[2]/available; volatile activeRxBuffer=2; owner=nullptr; core=tskNO_AFFINITY; rxEnabled=false,txEnabled=false,gpioConfigured=false. Очереди передают владение ISR/task, через границу идут только индексы. SDK-каналы/encoder выделяются отдельно.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

### `ESP32OT::Transport::RmtDecodedFrame`

Значение декодера, методов нет: State state=RESPONSE_WAITING; uint32_t data=0,elapsedUs=0,lastSampleUs=0; uint8_t bits=0; bool timingValid=true. elapsedUs/lastSampleUs — микросекунды внутри захвата; bits считает0..32, state отмечает принятие полного стопового перехода.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

### `ESP32OT::Transport::RmtPulse`

Локальное представление импульса, методов нет: uint32_t durationUs=0; bool level=false. Виртуальный фильтр может объединять длительности без изменения принятых RMT-символов.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

### `ESP32OT::Transport::RmtPhaseCandidate`

Локальный кандидат фазы, методов нет: uint32_t data=0;uint8_t halfIndex=0;bool ambiguous=false. Фиксированные фронты по два элемента отслеживают Manchester-фазы и помечают слияние разных данных в одной фазе; без динамической памяти и выбора по ID/чётности.

Исходник: [Transport/RmtDriver.cpp](../src/Transport/RmtDriver.cpp).

#### `static RmtDecodedFrame decodeRmtFrameLegacy(const rmt_symbol_word_t* symbols, size_t symbolCount, RmtDriver::State state, uint32_t data, uint8_t bits, bool timingValid)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Быстрый edge-декодер с прежними допусками partial/start/sample/stop. Входы включают предыдущие частичные биты и timing; возвращает новый результат. Idle не имеет конечного перехода и не создаёт стоповый бит. Полные данные могут сохраниться с timingValid=false для диагностики.

```cpp
auto frame = decodeRmtFrameLegacy(symbols, count, state, data, bits, timingValid);
```

#### `static bool readRmtPulse(const rmt_symbol_word_t* symbols, size_t symbolCount, size_t& pulseIndex, RmtPulse& pulse)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Читает следующую level/duration половину RMT-символа; сдвигает pulseIndex, false на границе,нулевой длительности или terminal idle. Длительность pulse в мкс; массив не меняется.

```cpp
while (readRmtPulse(symbols, count, pulseIndex, pulse)) { /* consume */ }
```

#### `static bool readRmtFilteredPulse(const rmt_symbol_word_t* symbols, size_t symbolCount, size_t& pulseIndex, RmtPulse& pulse)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Читает виртуальный импульс и объединяет внутренний выброс противоположного уровня до2мкс, после которого вернулся исходный уровень. Сохраняет общее время и фиксированный захват. False в конце; широкого ограничения длительностей нет.

```cpp
while (readRmtFilteredPulse(symbols, count, pulseIndex, pulse)) { /* clock samples */ }
```

#### `static bool decodeRmtFramePhase(const rmt_symbol_word_t* symbols, size_t symbolCount, RmtDecodedFrame& frame)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Пытается восстановить полный свежий захват: оценивает независимый медианный bit clock по парам разных уровней, проверяет67 half-cell, затем двигает ограниченные фазовые кандидаты. Успех требует однозначного Manchester-разбора и обновляет frame. False оставляет fallback вызывающего. Фиксированные stack-массивы; чётность/тип/ожидаемый ID не выбирают кандидата.

```cpp
if (decodeRmtFramePhase(symbols, count, frame)) { /* complete timing decode */ }
```

#### `static RmtDecodedFrame decodeRmtFrame(const rmt_symbol_word_t* symbols, size_t symbolCount, RmtDriver::State state, uint32_t data, uint8_t bits, bool timingValid)`

Деталь реализации. Пример — контекстный псевдокод внутри реализации; приложение не вызывает эту точку входа.

Запускает быстрый legacy-разбор; только для нового полного захвата из READY/RESPONSE_WAITING при неполном/ошибочном/timing-invalid результате пробует однозначное фазовое восстановление. Совместимость частичных захватов и исходные захваченные слова сохраняются.

```cpp
auto decoded = decodeRmtFrame(symbols, count, state, data, bits, timingValid);
```
