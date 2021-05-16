#pragma once
#include <Arduino.h>

#include <OneWire.h>

class ControlOneWire {
  public:
    ControlOneWire(byte pin, byte maxCountSensor); //конструктор
    void getAddress(byte *address, byte sensor);    //получить адрес датчика
    void deleteAddress(byte sensor);                //удалить адрес датчика
    void moveAddress(byte sensor1, byte sensor2);   //поменять местами адреса датчика1 и датчика2
    void searchStatusAndAddress(); //найти все адреса подключенных датчиков
    void initResult();                              //дать команду всем датчикам измерять температуру
    void editTimeWait(unsigned int timeWait);       //изменить время ожидания после initResult
    unsigned int getCountSensor();                          //получить кол-во адресов в списке
    byte getMaxCountSensor();                       //получить макс. разрешенное кол-во адресов в списке
    byte getStatusAddress(byte sensor);
    unsigned int getResult(byte sensor);                   //получить температуру опр. датчика

  private:
    OneWire _oneWire;
    byte _maxCountSensor;
    unsigned int _countSensor;
    byte *_allAddress;        //адреса, каждые 8 ячеек = 1 адрес
    byte *_allStatusAddress;  //0-ячейка пустая || 1-ячейка занята, подкл. нет || 2-ячейка занята, подкл. есть
    unsigned long time;
    unsigned int _timeWait = 75;

    void pasteAddress(byte *massive1, byte sensor, byte *massive2);
    void copyAddress(byte *massive1, byte sensor, byte *massive2);
    bool compareAddress(byte *massive1, byte sensor, byte *massive2);
};

ControlOneWire::ControlOneWire(byte pin, byte maxCountSensor) {
  _oneWire = OneWire(pin);
  _maxCountSensor = maxCountSensor;
  _allStatusAddress = new byte[_maxCountSensor];
  _allAddress = new unsigned byte[_maxCountSensor * 8];
  memset(_allAddress, 0, _maxCountSensor * 8);
}

bool isLimit = 0;
void ControlOneWire::searchStatusAndAddress() {
  byte address[8];
    isLimit = 0;
    _countSensor = 0;
    _oneWire.reset_search();
    memset(_allStatusAddress, 255, _maxCountSensor);
    //ищем все пустые ячейки в _allAddress
    for (byte sensor = 0; sensor < _maxCountSensor; sensor++) {
      byte nullAddress[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      _allStatusAddress[sensor] = !compareAddress(_allAddress, sensor, nullAddress);
    }
  //поиск всех адресов и их статусов
  for (; !isLimit && _oneWire.search(address);) {
    bool isFound = 0;
    _countSensor++;
    for (byte sensor = 0; sensor < _maxCountSensor && !isFound; sensor++) {
      if (compareAddress(_allAddress, sensor, address)) {
        _allStatusAddress[sensor] = 2;
        isFound = 1;
      }
    }
    if (!isFound) {
      for (byte sensor = 0; sensor < _maxCountSensor && !isFound; sensor++) {
        if (_allStatusAddress[sensor] == 0) {
          pasteAddress(_allAddress, sensor, address);
          _allStatusAddress[sensor] = 2;
          isFound = 1;
        }
      }
    }
    isLimit = !isFound;
  }
}

void ControlOneWire::getAddress(byte *address, byte sensor) {
  copyAddress(_allAddress, sensor, address);
}

void ControlOneWire::deleteAddress(byte sensor) {
  byte address[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  pasteAddress(_allAddress, sensor, address);
  _allStatusAddress[sensor] = 0;
  _countSensor -= 1;
}

void ControlOneWire::moveAddress(byte sensor1, byte sensor2) {
  byte address1[8], address2[8];
  byte statusAddress1, statusAddress2;
  copyAddress(_allAddress, sensor1, address1);
  copyAddress(_allAddress, sensor2, address2);
  pasteAddress(_allAddress, sensor1, address2);
  pasteAddress(_allAddress, sensor2, address1);
  statusAddress1 = _allStatusAddress[sensor1];
  statusAddress2 = _allStatusAddress[sensor2];
  _allStatusAddress[sensor1] = statusAddress2;
  _allStatusAddress[sensor2] = statusAddress1;
}

void ControlOneWire::editTimeWait(unsigned int timeWait) {
  _timeWait = timeWait;
}

byte ControlOneWire::getStatusAddress(byte sensor) {
  return _allStatusAddress[sensor];
}

unsigned int ControlOneWire::getCountSensor() {
  return _countSensor;
}

byte ControlOneWire::getMaxCountSensor() {
  return _maxCountSensor;
}

void ControlOneWire::initResult() {
  _oneWire.reset();
  _oneWire.write(0xCC);
  _oneWire.write(0x44);
  time = millis();
}

unsigned int ControlOneWire::getResult(byte sensor) {
  for (; (millis() - time) > _timeWait;);
  unsigned int result = 0;
  if (_allStatusAddress[sensor] == 2) {
    byte data[2], address[8];
    getAddress(address, sensor);
    _oneWire.reset();
    _oneWire.select(address);
    _oneWire.write(0xBE);
    data[0] = _oneWire.read();
    data[1] = _oneWire.read();
    result = ((data[1] << 8) | data[0]); //(data[1] << 8) | data[0]) ~(data[1] >> 7) ? (((data[1] << 8) | data[0]) * 0.0625) : (~((data[1] << 8) | data[0]) * -0.0625);
  }
  return result;
}

//Вспомогательные методы:
bool ControlOneWire::compareAddress(byte *massive1, byte sensor, byte *massive2) {
  byte index;
  for (index = 0; index < 8 && massive1[(sensor * 8) + index] == massive2[index]; index++);
  return index == 8;
}

void ControlOneWire::pasteAddress(byte *massive1, byte sensor, byte *massive2) {
  for (byte index = 0; index < 8; index++) {
    massive1[(sensor * 8) + index] = massive2[index];
  }
}

void ControlOneWire::copyAddress(byte *massive1, byte sensor, byte *massive2) {
  for (byte index = 0; index < 8; index++) {
    massive2[index] = massive1[(sensor * 8) + index];
  }
}
