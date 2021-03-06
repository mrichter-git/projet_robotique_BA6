#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#define NO_COLOR	0
#define ROUGE		1
#define VERT		2
#define	BLEU		3

/* fonction:  renvoie la couleur moyenne trouv?e
 * arguments: aucun
 * return:    uint8_t: couleur 0 = aucunce couleur dominante, 1 = rouge, 2 = vert, 3 = bleu
 */
uint8_t get_couleur(void);

/* fonction:  permet de r?initialiser les compteurs de couleur
 * arguments: aucun
 * return:    aucun
 */
void reset_couleur(void);

/* fonction:  capture une image et trouve sa couleur dominante qui sera stock?e
 * arguments: aucun
 * return:    aucun
 */
void capture_couleur(void);

/* fonction:  init la camera selon les param?tres d?sir?s
 * arguments: aucun
 * return:    aucun
 */
void camera_init(void);

#endif /* PROCESS_IMAGE_H */
