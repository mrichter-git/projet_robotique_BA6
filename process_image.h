#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

/* capture une image et renvoie la couleur dominante vue par la camera.
 *  0 = aucunce couleur dominante
 *  1 = rouge
 *  2 = vert
 *  3 = bleu
 */
uint8_t get_couleur(void);

#define NO_COLOR	0
#define ROUGE		1
#define VERT		2
#define	BLEU		3


#endif /* PROCESS_IMAGE_H */
