#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

/* fonction:  renvoie la couleur moyenne trouvée
 * arguments: aucuns
 * return:    uint8_t: couleur 0 = aucunce couleur dominante, 1 = rouge, 2 = vert, 3 = bleu
 */
uint8_t get_couleur(void);

/* fonction:  permet de réinitialiser les compteurs de couleur
 * arguments: aucun
 * return:    aucun
 */
void reset_couleur(void);

/* fonction:  capture une image et trouve sa couleur dominante qui sera stockée
 * arguments: aucun
 * return:    aucun
 */
void capture_couleur(void);

#define NO_COLOR	0
#define ROUGE		1
#define VERT		2
#define	BLEU		3


#endif /* PROCESS_IMAGE_H */
