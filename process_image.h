#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

/* renvoie la couleur du mur du fond lue par la camera
 *  0 = aucunce couleur dominante
 *  1 = rouge
 *  2 = vert
 *  3 = bleu
 */
uint8_t get_colour(void);

//initialisation des threads de la camera
void process_image_start(void);


#endif /* PROCESS_IMAGE_H */
