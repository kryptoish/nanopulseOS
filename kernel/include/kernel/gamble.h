#ifndef GAMBLE_H
#define GAMBLE_H

/*
 * NanoPulse Casino - spend coins on cases to add error "skins" to a
 * session-only inventory. Balance and inventory reset on every boot.
 *
 * gamble_run()          - enter the casino screen. Blocks until Ctrl+C or Q.
 * gamble_inventory()    - print the session inventory at the shell.
 *                         sort_recent = 0 → sort by rarity (desc),
 *                         sort_recent = 1 → sort by acquisition (newest first).
 */
void gamble_run(void);
void gamble_inventory(int sort_recent);
void gamble_add_coins(unsigned int amount);

#endif
