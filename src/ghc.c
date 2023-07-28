/** @brief Serial: 6a86135c
 *  @file lab01.c
 */
#include <stdio.h>
#include <stdlib.h>
#include "ghcontrol.h"

int main(void){

	int logged;
	time_t now;
	struct readings creadings = {0};
    struct controls ctrl = {0};
	struct setpoints sets = {0};

	GhControllerInit();
	GhDisplayHeader("Darshan Prajapati");


	sets = GhSetTargets();
	while(1)
	{

		now = time(NULL);
        creadings = GhGetReadings();
		ctrl = GhSetControls(sets, creadings);
		GhDisplayAll(creadings,sets);
		logged = GhLogData("ghdata.txt",creadings);
        GhDisplayReadings(creadings);
        GhDisplayTargets(sets);
        GhDisplayControls(ctrl);
        GhDelay(GHUPDATE);


	}

       	//fprintf(stdout,"Press ENTER to continue...");
	//fgetc(stdin);

	return EXIT_FAILURE;
}
