/**
 * @file      Interpreter.h
 * @brief     Real Time Operating System for Labs 2, 3 and 4
 * @details   EE445M/EE380L.6
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      Jan 5, 2020

 ******************************************************************************/



#ifndef INTERPRETER_H
#define INTERPRETER_H



/**
 * @details  Run the user interface.
 * @param  none
 * @return none
 * @brief  Interpreter
 */
void Interpreter(void);
void Jitter(Jitter_t* J, uint8_t lcd_id);

void Interpreter_register_remote_thread(void);
void Interpreter_unregister_remote_thread(void);

#endif