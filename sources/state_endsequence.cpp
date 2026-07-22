#ifdef _WIN32
#include "windows.h"
#endif

#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "compat/sdl3_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "compat/SDL_net.h"

#include "F1Spirit.h"
#include "Vector.h"
#include "sound.h"
#include "List.h"
#include "auxiliar.h"
#include "2DCMC.h"
#include "GLTile.h"
#include "PlacedGLTile.h"
#include "RotatedGLTile.h"
#include "keyboardstate.h"
#include "CPlayer.h"
#include "CCar.h"
#include "2DCMC.h"
#include "RoadPiece.h"
#include "track.h"
#include "RacingCCar.h"
#include "PlayerCCar.h"
#include "EnemyCCar.h"
#include "GameParameters.h"
#include "ReplayInfo.h"
#include "F1SpiritGame.h"
#include "F1SpiritApp.h"
#include "randomc.h"
#include "debug.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

extern int N_SFX_CHANNELS;
extern bool sound;
extern TRanrotBGenerator *rg;


#define ENDSEQUENCE_CTNT 100
#define ENDSEQUENCE_CTNT2   7





int F1SpiritApp::endsequence_cycle(KEYBOARDSTATE *k)
{
	if (state_cycle == 0) {

		/*
		#ifdef _WIN32
		        if (sound) {
		            Stop_playback();
		            N_SFX_CHANNELS = Resume_playback(N_SFX_CHANNELS, 0);
		        } // if
		#endif
		*/
		Sound_create_music("sound/game_ending", -1);
		Sound_music_volume(127);

        endsequence_images[0] = new GLTile("graphics/end0-sky.png", 0, 0, 380, 254);
        
        // stock car image
        endsequence_images[1] = new GLTile("graphics/end1.png", 0, 0, 380, 254);

		// not being used right now (rally image)
        endsequence_images[2] = new GLTile("graphics/end2.png", 0, 0, 380, 254);

		// rally car image
        endsequence_images[3] = new GLTile("graphics/end3.png", 0, 0, 380, 254);
        
        endsequence_images[4] = new GLTile("graphics/end0-star.png", 0, 0, 11, 14);
        endsequence_images[5] = new GLTile("graphics/end8.png", 0, 0, 474, 350);
        endsequence_images[6] = new GLTile("graphics/end7.png", 0, 0, 265, 93);
        endsequence_images[7] = new GLTile("graphics/end6.png", 0, 0, 380, 254);
        endsequence_images[8] = new GLTile("graphics/end5.png", 0, 0, 380, 254);
        
        // enduro car image
        endsequence_images[9] = new GLTile("graphics/end4.png", 0, 0, 380, 254);

        // pilot with girls
        endsequence_images[10] = new GLTile("graphics/end10.png", 0, 0, 380, 254);
        // pilot with champagne
        endsequence_images[11] = new GLTile("graphics/end11.png", 0, 0, 380, 254);
        
        // endsequence_images[12] = new GLTile("graphics/end0-rosto_senna-1.png", 0, 0, 380, 254);
        endsequence_images[12] = new GLTile("graphics/end0-pilot-1.png", 0, 0, 380, 254);
        endsequence_images[13] = new GLTile("graphics/end0-pilot-2.png", 0, 0, 380, 254);
        endsequence_images[14] = new GLTile("graphics/end0-pilot-3.png", 0, 0, 380, 254);
        endsequence_images[15] = new GLTile("graphics/end0-pilot-4.png", 0, 0, 380, 254);


        
        
	} 

	switch (endsequence_state) {

		case 0:  /* FADING IN: */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT) {
				endsequence_state = 1;
				endsequence_timmer = 0;
				cont_animation = 0;
			} 

			break;

		case 1:  /* SHOWING PILOT */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 2;
				endsequence_timmer = 0;
				
			} 

			break;

		case 2:  /* SHOWING STOCK CAR: */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 3;
				endsequence_timmer = 0;
			} 

			break;

		case 3:  /* SHOWING RALLY CAR 1: */
            // scene eliminated (for now)
            /*
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 4;
				endsequence_timmer = 0;
			} 
            */
            	endsequence_state = 4;
				endsequence_timmer = 0;
			break;

        case 4:  /* SHOWING RALLY CAR 2: */ 
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 5;
				endsequence_timmer = 0;
			} 

			break;

        case 5:  /* SHOWING ENDURANCE CAR: */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 6;
				endsequence_timmer = 0;
			} 

			break;



        case 6:  /* SHOWING WINNING F1 RACE: */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 7;
				endsequence_timmer = 0;
			} 

			break;


		case 7:  /* SHOWING PILOT WITH CUP */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 8;
				endsequence_timmer = 0;
			} 

			break;

		case 8:  /* SHOWING PILOT OPENING CHAMPAGNE: */ 
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 9;
				endsequence_timmer = 0;
				cont_animation = 0;

			} 

			break;


		case 9:  /* SHOWING PILOT WITH GIRLS: */ 
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 10;
				endsequence_timmer = 0;
				cont_animation = 0;

			} 

			break;

		case 10:  /* SHOWING PILOT */ 
			endsequence_timmer++;

			if (endsequence_timmer >= 0.9*(ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT)) {
				endsequence_state = 11;
				endsequence_timmer = 0;
				cont_animation = 0;

			} 

			break;

		case 11:  /* SHOWING CREDITS: */ 
			endsequence_timmer++;

			// if (endsequence_timmer >= ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
            if (endsequence_timmer >= 6.6*ENDSEQUENCE_CTNT2*ENDSEQUENCE_CTNT) {
				endsequence_state = 12;
				endsequence_timmer = 0;
			} 

			break;


		case 12:   /* FADING OUT: */
			endsequence_timmer++;

			if (endsequence_timmer >= ENDSEQUENCE_CTNT)
				endsequence_state = 13;

			Sound_music_volume(int(127.0F*float(ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT));

			break;

		case 13:  /* finish */
			break;
	} 

	if (k->keyboard[SDLK_ESCAPE] && !k->old_keyboard[SDLK_ESCAPE]) {
		if (endsequence_state != 12) {
			endsequence_state = 12;
			endsequence_timmer = 0;
		} 
	} 

	if (endsequence_state == 13) {
		delete endsequence_images[0];
		delete endsequence_images[1];
		delete endsequence_images[2];
		delete endsequence_images[3];
		delete endsequence_images[4];
		delete endsequence_images[5];
		delete endsequence_images[6];
		delete endsequence_images[7];
        delete endsequence_images[8];
        delete endsequence_images[9];
        delete endsequence_images[10];
        delete endsequence_images[11];
        delete endsequence_images[12];
        delete endsequence_images[13];
        delete endsequence_images[14];
        delete endsequence_images[15];




		endsequence_images[0] = 0;
		endsequence_images[1] = 0;
		endsequence_images[2] = 0;
		endsequence_images[3] = 0;
		endsequence_images[4] = 0;
		endsequence_images[5] = 0;
		endsequence_images[6] = 0;
		endsequence_images[7] = 0;
		endsequence_images[8] = 0;
        endsequence_images[9] = 0;
        endsequence_images[10] = 0;
        endsequence_images[11] = 0;
        endsequence_images[12] = 0;
        endsequence_images[13] = 0;
        endsequence_images[14] = 0;
        endsequence_images[15] = 0;



		menu_fading = -1;
		menu_fading_ctnt = 25;
		menu_current_menu = 4;

		return endsequence_returnstate;
	} 

	return APP_STATE_ENDSEQUENCE;
} 


void F1SpiritApp::endsequence_draw(void)
{
	// glClearColor(0.8F, 0.8F, 0.8F, 1);
	glClearColor(0.0F, 0.0F, 0.0F, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
   



	switch (endsequence_state) {

		case 0:  /* FADING IN: */
			            
            glEnable(GL_COLOR_MATERIAL);

			{
				float f = 0;
				f = float(abs(ENDSEQUENCE_CTNT - endsequence_timmer)) / ENDSEQUENCE_CTNT;
				glColor4f(0, 0, 0, f);
			}

			glNormal3f(0.0, 0.0, 1.0);

			glBegin(GL_QUADS);
			glVertex3f(0, 0, 0);
			glVertex3f(0, 480, 0);
			glVertex3f(640, 480, 0);
			glVertex3f(640, 0, 0);
			glEnd();
			break;

		case 1: { /* SHOWING pilot: */
		
                // draw frame and text : "Memories of Formula 1"
			    endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			    endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);
			
			
            	float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

                // draw the sky
                endsequence_images[0]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                
                // draw pilot with his eyes opened
                endsequence_images[15]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

                /*
                teste: fazer as estrelas brilharem apenas em vez de se mexer.
                nota: nao est� funcionando ainda
                float f2 = 1.0F;
   
   				if (endsequence_timmer % 7*ENDSEQUENCE_CTNT < 500)
					f2 = (float(endsequence_timmer)) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer % 7*ENDSEQUENCE_CTNT > 500)
					f2 = (float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer)) / ENDSEQUENCE_CTNT;

				if (f2 < 0)
					f2 = 0;

				if (f2 > 1)
					f2 = 1;

                
                // draw the stars
                endsequence_images[4]->draw(1, 1, 1, f2, 150 , 100, 0, 0, 1);
                endsequence_images[4]->draw(1, 1, 1, f2, 210 , 190, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f2, 280 , 150, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f2, 340 , 120, 0, 0, 1);

				*/

                // draw the stars
                endsequence_images[4]->draw(1, 1, 1, f, 150 + cont_animation, 100, 0, 0, 1);
                endsequence_images[4]->draw(1, 1, 1, f, 210 + cont_animation, 190, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f, 280 + cont_animation, 150, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f, 340 + cont_animation, 120, 0, 0, 1);
				
				
				
				// pilot's eye animation
                if ((endsequence_timmer % 300) > 250)  {
                                          
                       if ((endsequence_timmer % 300) < 260) 
                          endsequence_images[14]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 260 && (endsequence_timmer % 350) < 270) 
                          endsequence_images[13]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 270 && (endsequence_timmer % 350) < 280) 
                          endsequence_images[12]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 280 && (endsequence_timmer % 350) < 290) 
                          endsequence_images[13]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 290 && (endsequence_timmer % 350) < 300) 
                          endsequence_images[14]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                    }
                      
                if ((endsequence_timmer % 300) <= 250) 
                      endsequence_images[15]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                
                if (endsequence_timmer % 5 == 0)
                   cont_animation++;


				
			}

			break;

		case 2: { /* SHOWING STOCK CARS: */
			
               // draw frame and text : "Memories of Formula 1"
			   endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			   endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;


				if (endsequence_timmer > ENDSEQUENCE_CTNT &&
				        endsequence_timmer < (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT &&
				        (rg->IRandom(0, 23)) == 0) {
					endsequence_images[1]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

					
				} else {
					endsequence_images[1]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				} 
			}

			break;

		case 3: { /* SHOWING RALLY CAR 1: */
			
                // draw frame and text : "Memories of Formula 1"
			    endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			    endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;
                
				endsequence_images[2]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

			}

			break;

		case 4: { /* SHOWING RALLY CAR 2: */
			
                // draw frame and text : "Memories of Formula 1"
			    endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			    endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;
                
				endsequence_images[3]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

			}

			break;


		case 5: { /* SHOWING ENDURANCE CAR: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

 				endsequence_images[9]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				
			}

			break;


		case 6: { /* SHOWING WINNING F1 RACE: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

            endsequence_images[8]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				
			}

			break;


         case 7: { /* SHOWING PILOT WITH CUP: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

				endsequence_images[7]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				
			}

			break;


		case 8: { /* SHOWING PILOT OPENING CHAMPAGNE: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

				endsequence_images[11]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				
			}

			break;

		case 9: { /* SHOWING PILOT WITH GIRLS: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > (ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

				endsequence_images[10]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

				
			}

			break;


		case 10: { /* SHOWING pilot: */

			endsequence_images[5]->draw(1, 1, 1, 1, 83, 30, 0, 0, 1);
			endsequence_images[6]->draw(1, 1, 1, 1, 182, 380, 0, 0, 1);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > 0.9*(ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

                // draw the sky
                endsequence_images[0]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                
                // draw pilot with eyes opened
                endsequence_images[15]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);

                // draw the stars
                endsequence_images[4]->draw(1, 1, 1, f, 150 + cont_animation, 100, 0, 0, 1);
                endsequence_images[4]->draw(1, 1, 1, f, 210 + cont_animation, 190, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f, 280 + cont_animation, 150, 0, 0, 1);
				endsequence_images[4]->draw(1, 1, 1, f, 340 + cont_animation, 120, 0, 0, 1);
				
				// pilot's eye animation
                if ((endsequence_timmer % 300) > 250)  {
                                          
                       if ((endsequence_timmer % 300) < 260) 
                          endsequence_images[14]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 260 && (endsequence_timmer % 350) < 270) 
                          endsequence_images[13]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 270 && (endsequence_timmer % 350) < 280) 
                          endsequence_images[12]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 280 && (endsequence_timmer % 350) < 290) 
                          endsequence_images[13]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                       else if ((endsequence_timmer % 300) >= 290 && (endsequence_timmer % 350) < 300) 
                          endsequence_images[14]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                    }
                      
                if ((endsequence_timmer % 300) <= 250) 
                      endsequence_images[15]->draw(1, 1, 1, f, 130, 80, 0, 0, 1);
                
                if (endsequence_timmer % 5 == 0)
                   cont_animation++;

				
			}

			break;

		case 11: { /* SHOWING CREDITS: */
                
                glClearColor(0.0F, 0.0F, 0.0F, 1);
                glClear(GL_COLOR_BUFFER_BIT);

				float f = 1.0F;

				if (endsequence_timmer < ENDSEQUENCE_CTNT)
					f = float(endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (endsequence_timmer > 9*(ENDSEQUENCE_CTNT2 - 1)*ENDSEQUENCE_CTNT)
					f = float(ENDSEQUENCE_CTNT2 * ENDSEQUENCE_CTNT - endsequence_timmer) / ENDSEQUENCE_CTNT;

				if (f < 0)
					f = 0;

				if (f > 1)
					f = 1;

				GLTile *t;

                
                SDL_Surface *sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"ORIGINAL GAME", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"ORIGINAL GAME", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);
				
				// endsequence_images[4]->draw(1, 1, 1, f, 150 + cont_animation, 100, 0, 0, 1);

                t->draw(1, 1, 1, f, 216, 494 - ( endsequence_timmer / 3), 0, 0, 1);
				// t->draw(1, 1, 1, f, 96, 64, 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Konami 1987", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Konami 1987", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 96, 0, 0, 1);
                t->draw(1, 1, 1, f, 232, 516 - (endsequence_timmer / 3), 0, 0, 1);
				delete t;
				
				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"REMAKE", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"REMAKE", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 128, 0, 0, 1);
                t->draw(1, 1, 1, f, 270, 558 - (endsequence_timmer / 3), 0, 0, 1);
             
				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"BRAINGAMES 2009", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"BRAINGAMES 2009", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 144, 0, 0, 1);
                t->draw(1, 1, 1, f, 200, 580 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"CODING", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"CODING", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 176, 0, 0, 1);
                t->draw(1, 1, 1, f, 270, 622 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Santi Onta��n", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Santi Onta��n", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 96, 240, 0, 0, 1);
                t->draw(1, 1, 1, f, 216, 644 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"GRAPHICS", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"GRAPHICS", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 272, 0, 0, 1);
				t->draw(1, 1, 1, f, 256, 686 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Ramon Verlinden", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Ramon Verlinden", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 304, 0, 0, 1);
                t->draw(1, 1, 1, f, 200, 708 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Miikka Poikela", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Miikka Poikela", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 304, 0, 0, 1);
                t->draw(1, 1, 1, f, 208, 730 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"David Castelao", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"David Castelao", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 304, 0, 0, 1);
                t->draw(1, 1, 1, f, 208, 752 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Maur�cio Braga", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Maur�cio Braga", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 320, 0, 0, 1);
                t->draw(1, 1, 1, f, 208, 774 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"MUSIC & SFX", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"MUSIC & SFX", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 372, 0, 0, 1);
                t->draw(1, 1, 1, f, 232, 816 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Jorrith Schaap", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Jorrith Schaap", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 372, 0, 0, 1);
                t->draw(1, 1, 1, f, 208, 838 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"TRACKS", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"TRACKS", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 372, 0, 0, 1);
                t->draw(1, 1, 1, f, 272, 880 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;



				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Santi Onta��n", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Santi Onta��n", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 216, 902 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"BETA TESTING", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"BETA TESTING", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 224, 944 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"All of the above", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"All of the above", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 192, 966 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Jason Eames", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Jason Eames", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 232, 988 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Joram van Hartingsveldt", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Joram Van Hartingsveldt", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 136, 1010 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Patrick van Arkel", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Patrick van Arkel", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 184, 1032 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Pakoto", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Pakoto", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 271, 1054 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Chocobo2k", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Chocobo2k", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 248, 1076 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Silver Sword", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Silver Sword", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 224, 1098 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Valerian", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Valerian", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 256, 1120 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"theNestruo", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"theNestruo", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 240, 1142 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Lars the 18th", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Lars the 18th", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 216, 1164 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Konamito", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Konamito", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 256, 1186 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"AcesHigh", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"AcesHigh", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 256, 1208 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Special Thanks", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Special Thanks", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 208, 1250 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Jason Eames", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Jason Eames", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 232, 1272 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Joram van Hartingsveldt", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Joram van Hartingsveldt", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 136, 1294 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Juhani Vuontio", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Juhani Vuontio", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 208, 1316 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Lars the 18th", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Lars the 18th", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 216, 1338 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Manuel Bilderbeek", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Manuel Bilderbeek", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 184, 1360 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Valerian", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Valerian", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 256, 1382 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Jorrith Schaap", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Jorrith Schaap", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 208, 1404 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Ootini", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Ootini", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 272, 1426 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;
				
         		
                sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"Presented by BrainGames", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"Presented by BrainGames", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

                t->draw(1, 1, 1, f, 136, 1530 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;


                /*
				
				// old code (credits without all the names in Jorito's F1Spirit website and scroll)
				
                SDL_Surface *sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"ORIGINAL KONAMI CREW", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"ORIGINAL KONAMI CREW", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);
				
				// endsequence_images[4]->draw(1, 1, 1, f, 150 + cont_animation, 100, 0, 0, 1);

                t->draw(1, 1, 1, f, 96, 64 - ( endsequence_timmer / 3), 0, 0, 1);
				// t->draw(1, 1, 1, f, 96, 64, 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"CODE: T.ADACHI, M.OZAWA, R.SAGISAKA", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"CODE: T.ADACHI, M.OZAWA, R.SAGISAKA", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 96, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 96 - (endsequence_timmer / 3), 0, 0, 1);
				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"GRAPHICS: H.MAKITANI, T.SHIMIZU,", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"GRAPHICS: H.MAKITANI, T.SHIMIZU,", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 128, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 128 - (endsequence_timmer / 3), 0, 0, 1);
             
				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"          B.KINOSHITA", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"          B.KINOSHITA", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 144, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 144 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"SOUND: K.UEHARA, M.SASAKI, M.FURUKAWA", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"SOUND: K.UEHARA, M.SASAKI, M.FURUKAWA", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 176, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 176 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"BRAIN GAMES CREW", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"BRAIN GAMES CREW", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 96, 240, 0, 0, 1);
                t->draw(1, 1, 1, f, 96, 240 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"CODE: POPOLON", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"CODE: POPOLON", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 272, 0, 0, 1);
				t->draw(1, 1, 1, f, 16, 272 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"GRAPHICS: RAMONMSX, MP83", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"GRAPHICS: RAMONMSX, MP83", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 304, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 304 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"          VALERIAN, PICILI", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"          VALERIAN, PICILI", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 320, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 320 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"          IAMWEASEL, DFCASTELAO", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"          IAMWEASEL, DFCASTELAO", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 336, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 336 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;

				sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, get_text_width_bmp((unsigned char *)"SOUND: JORITO", font, 0), font->h, 32, RMASK, GMASK, BMASK, AMASK);

				print_left_bmp((unsigned char *)"SOUND: JORITO", font, sfc, 0, 0, 0);

				t = new GLTile(sfc);

				// t->draw(1, 1, 1, f, 16, 372, 0, 0, 1);
                t->draw(1, 1, 1, f, 16, 372 - (endsequence_timmer / 3), 0, 0, 1);

				delete t;
				*/
				
			}

			break;
        
        
     
		case 12:  /* FADING OUT: */
			glEnable(GL_COLOR_MATERIAL);

			{
				float f = 0;
				f = float(abs(endsequence_timmer)) / 100.0F;
				glColor4f(0, 0, 0, f);
			}

			glNormal3f(0.0, 0.0, 1.0);

			glBegin(GL_QUADS);
			glVertex3f(0, 0, 0);
			glVertex3f(0, 480, 0);
			glVertex3f(640, 480, 0);
			glVertex3f(640, 0, 0);
			glEnd();
			break;

		case 13:  /* finish */
			break;
	} 

	if (title_state != 0) {} 

} 

