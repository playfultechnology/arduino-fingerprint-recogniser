

#include "flasher.h"


void Flasher::begin(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
  _off();
}

void Flasher::flash() {
  this->flash(-1);
}

void Flasher::flash(int32_t count) {
  _counter = count;
  _flashing = true;
  _on();
}

void Flasher::stop() {
  _flashing = false;
  _off();
}

void Flasher::update() {
  if (_flashing) {
    if ((millis() - _timer) > _flashtime) {
      _set(!_state);
      if (!_state && _counter > -1 && --_counter == 0) {
        stop();
      }
    }
  } else if (_state) {
    _off();
  }
}


void Flasher::_set(bool state) {
  digitalWrite(_pin, state ? _onstate : _offstate); 
  _state = state;
  _timer = millis();
}

void Flasher::_on() {
  _set(true);
}

void Flasher::_off() {
  _set(false);
}

