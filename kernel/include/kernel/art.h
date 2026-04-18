#ifndef ART_H
#define ART_H

/*
 * Enter graphics mode, render a device-unique fingerprint, and block until
 * the user presses Ctrl+C or ESC. On return the text console is restored.
 */
void art_run(void);

#endif
