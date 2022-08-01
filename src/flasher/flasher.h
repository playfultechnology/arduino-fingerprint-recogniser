/** Non-blocking LED flasher library.
 *
 * (c) 2020 Ali Afshar
 *
 * MIT
 */


#include <stdint.h>
#include "Arduino.h"


class Flasher {

  public:
    void           begin(uint8_t pin);
    void           flash();
    void           flash(int32_t);
    void           stop();
    void           update();

  private:
    uint8_t        _pin;
    uint8_t        _onstate = LOW;
    uint8_t        _offstate = HIGH;
    bool           _state = false;
    unsigned long  _timer = 0;
    int32_t        _counter = 0;
    uint32_t       _flashtime = 200;
    bool           _flashing = false;
    void           _on();
    void           _off();
    void           _set(bool state);
    void           _update();

};



