/*
 * sound.h
 *
 *  Created on: Jul 17, 2014
 *      Author: zacaj
 */

#ifndef SOUND_H_
#define SOUND_H_

void initSound();
void updateSound();

extern uint8_t paused;
int loadSound(char *file);

#endif /* SOUND_H_ */
