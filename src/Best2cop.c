#include "../include/Best2cop.h"



int Best2cop(Pfront_t*** pfront, Dict_t*** pf, SrGraph_t* graph, int src, my_m1 cstrM1, 
            my_m2 cstrM2, my_m1 dictSize, char analyse, int** iters)
{
    //Start of init
    int maxIter = SEG_MAX;
    my_m2* minIgp = malloc(graph->nbNode * sizeof(my_m2));

    if (analyse) {
        maxIter = MAX(maxIter, 10 * SEG_MAX);
    }

    int nbIter = 1;

    (*pfront) = malloc((maxIter + 1) * sizeof(Pfront_t*));
    ASSERT(pfront, -1);

    for (int i = 0 ; i <= maxIter ; i++) {
        (*pfront)[i] = malloc(graph->nbNode * sizeof(Pfront_t));
        ASSERT((*pfront)[i], -1);

        for (int j = 0 ; j < graph->nbNode ; j++) {
            Pfront_init(&(*pfront)[i][j], dictSize);
        }
    }

    Pfront_insert_key(&(*pfront)[0][src], 0);

    Dict_t* dist = malloc(graph->nbNode * sizeof(Dict_t));
    ASSERT(dist, -1);

    for (int i = 0 ; i < graph->nbNode ; i++) {
        Dict_init(&dist[i], dictSize);
    }

    Dict_add(&dist[src], 0, 0, src);

    Extendable_list_t* extendable = NULL;

    extendable = Extendable_list_new(NULL, src, NULL);
    extendable->ext = Extendable_new(0, 0, NULL);

    Extendable_t** nextextendable = malloc(graph->nbNode * sizeof(Extendable_t*));
    ASSERT(nextextendable, -1);

    for (int i = 0 ; i < graph->nbNode ; i++) {
        nextextendable[i] = NULL;
    }

    my_m2 tmp_igp = 0;


    (*pf) = malloc((maxIter+ 1) * sizeof(Dict_t*));
    ASSERT((*pf), -1);

    for (int i = 0 ; i <= maxIter ; i++) {
        (*pf)[i] = malloc(graph->nbNode * sizeof(Dict_t));
        ASSERT((*pf)[i], -1);
        for (int j = 0 ; j < graph->nbNode ; j++) {
            Dict_init((*pf)[i] + j, dictSize);
        }
    }

    Dict_add((*pf)[src], 0, 0, src);

    if (iters != NULL) {
        for (int i = 0 ; i < graph->nbNode ; i++) {
            (*iters)[i] = -1;
            minIgp[i] = INF;
        }
        (*iters)[src] = 0;
    }

    
    while (extendable != NULL && nbIter <= maxIter) {
            
        #pragma omp parallel for
        for (int dst = 0 ; dst < graph->nbNode ; dst++) {

            if (dst == src) {
                continue;
            }

            Dict_t pf_cand;
            Dict_init(&pf_cand, dictSize);
            Pfront_t pfcandlist;
            Pfront_init(&pfcandlist, dictSize);
            int t = 0;
            my_m1 imax = 0;

            //extend paths
            Best2cop_extend_path(dst, extendable, &pf_cand, &pfcandlist, dist + dst, graph, &t, &imax, cstrM1, cstrM2);
            nextextendable[dst] = NULL;

            Best2cop_cpt_extendable_paths(nextextendable + dst, pfront, &pf_cand, dist + dst, &pfcandlist, t, imax, nbIter, dst, &(*pf)[nbIter][dst]);

            Dict_free(&pf_cand);
            Pfront_free(&pfcandlist);
        }

        Extendable_list_free(extendable);
        extendable = NULL;

        for (int i = 0 ; i < graph->nbNode ; i++) {
            //printf("iter : %d -> node %d\n", nbIter, i);
            if (nextextendable[i] != NULL) {
                if (iters != NULL) {
                    if (analyse == ANALYSE_DCLC) {
                        //Extendable_print(nextextendable[i]);
                        //
                        tmp_igp = nextextendable[i]->infos.m2;
                        if (tmp_igp < minIgp[i]) {
                            (*iters)[i] = nbIter;
                            minIgp[i] = tmp_igp;
                        }
                        
                    }

                    if (analyse == ANALYSE_2COP) {
                        (*iters)[i] = nbIter;
                        // if (src == 208 && i == 8) {
                        //     printf("for node %d ", i);
                        //     Extendable_print(nextextendable[i]);
                        //     printf("\n");
                        // }
                    }
                }
                extendable = Extendable_list_new(extendable, i, NULL);
                extendable->ext = Extendable_copy(nextextendable[i]);
                Extendable_free(nextextendable[i]);
            }
        }
        //printf("nb iters = %d\n", nbIter);

        nbIter++;
    }

    

    Extendable_list_free(extendable);

    for (int j = 0 ; j < graph->nbNode ; j++) {
        Dict_free(&dist[j]);
    }
    free(dist);
    free(nextextendable);
    free(minIgp);

    return nbIter;
}


my_m2 update_min_igp(my_m2 old, Extendable_t* nextext) {
    if (nextext == NULL) {
        return old;
    }

    return MIN(nextext->infos.m2, update_min_igp(old, nextext->next));
}

void Best2cop_extend_path(int dst, Extendable_list_t* extendable, Dict_t* pf_cand, Pfront_t* pfcandlist, 
                            Dict_t* dist_v, SrGraph_t* graph, int* t, my_m1* imax, my_m1 c1, my_m2 c2)
{
    *imax = 0;
    *t = 0;
    for (Extendable_list_t* d_list = extendable ; d_list != NULL ; d_list = d_list->next) {
        short edgeSrc = d_list->node;
        for (Extendable_t* path = d_list->ext ; path != NULL ; path = path->next) {
            for (Edge_t* edge = graph->pred[dst][edgeSrc] ; edge != NULL ; edge = edge->next) {
                my_m1 d1v = path->infos.m1 + edge->m1;
                my_m2 d2v = path->infos.m2 + edge->m2;

                if (d1v < c1 && d2v < c2 && dist_v->paths[d1v] > d2v) {
                    Dict_add(dist_v, d1v, d2v, edgeSrc);
                    if (pf_cand->paths[d1v] == INF) {
                        Pfront_insert_key(pfcandlist, d1v);
                        *t = *t + 1;
                    }

                    Dict_add(pf_cand, d1v, d2v, 0);

                    *imax = MAX(*imax, d1v);
                }
            }
        }
    }
}


void Best2cop_cpt_extendable_paths(Extendable_t** nextextendable, Pfront_t*** pfront, 
                                    Dict_t* pf_cand, Dict_t* dist_v, __attribute__ ((unused)) Pfront_t* pfcandlist, __attribute__ ((unused)) int t, 
                                    int imax, int iter, int dst, Dict_t* pf)
{
    // if (t * log(t) + t + (*pfront)[iter-1][dst].heapSize < imax / 10) {
    //     if (dst == 8) {
    //         printf("On est dans select\n");
    //     }
    //     Best2cop_cpt_extendable_paths_select(nextextendable, pfront, pf_cand, dist_v, pfcandlist, iter, dst, pf);
    // } else {
    //     Best2cop_cpt_extendable_paths_all(nextextendable, pfront, pf_cand, dist_v, iter, dst, imax, pf);
    // }

    //printf("%d\n", t);
    Best2cop_cpt_extendable_paths_all(nextextendable, pfront, pf_cand, dist_v, iter, dst, imax, pf);
}


// void Best2cop_cpt_extendable_paths_select(Extendable_t** nextextendable, Pfront_t*** pfront, 
//                                     Dict_t* pf_cand, Dict_t* dist_v, Pfront_t* pfcandlist, int iter, int dst, ParetoFront_t** pf)
// {
//     Pfront_sort(pfcandlist);
//     Pfront_t* d1_it = Pfront_merge_sort(&(*pfront)[iter-1][dst], pfcandlist);
//     if (dst == 8) {
//         Pfront_print(d1_it, stdout);
//     }

//     my_m2 last_d2 = INF;
//     for (int i = 0 ; i < d1_it->heapSize ; i++) {
//         my_m1 d1 = d1_it->keys[i];
//         if (dist_v->paths[d1] < last_d2) {
//             last_d2 = dist_v->paths[d1];
//             Pfront_insert_key(&(*pfront)[iter][dst], d1);
//             (*pf) = ParetoFront_new((*pf), d1, dist_v->paths[d1]);
//             if (pf_cand->paths[d1] != INF) {
//                 (*nextextendable) = Extendable_new(d1, pf_cand->paths[d1], (*nextextendable));
//             }
//         }
//     }

//     Pfront_free(d1_it);
//     free(d1_it);
// }


void Best2cop_cpt_extendable_paths_all(Extendable_t** nextextendable, Pfront_t*** pfront, 
                                    Dict_t* pf_cand, Dict_t* dist_v, int iter, int dst, int imax, Dict_t* pf)
{
    UNUSED(imax);
    my_m2 last_d2 = INF;
    for (my_m1 i = 0 ; i < dist_v->size ; i++) {
        if (dist_v->paths[i] < last_d2) {
            last_d2 = dist_v->paths[i];
            Pfront_insert_key(&(*pfront)[iter][dst], i);
            if (pf_cand->paths[i] != INF) {
                Dict_add(pf, i, dist_v->paths[i], dist_v->preds[i]);
                (*nextextendable) = Extendable_new(i, pf_cand->paths[i], (*nextextendable));
            }
        }
    }
}


struct segment_list*** Dict_retreive_paths(Dict_t** d, SrGraph_t* sr, int maxiter, short src)
{
    struct segment_list*** sl = calloc(maxiter + 1, sizeof(struct segment_list**));
    for (int j = 0 ; j <= maxiter ; j++) {
        sl[j] = calloc(sr->nbNode, sizeof(struct segment_list*));
        for (int i = 0 ; i < sr->nbNode ; i++) {
            sl[j][i] = calloc(1000, sizeof(struct segment_list));
        }
    }
    short pred;
    struct segment_list* stack[10];
    int  stacksize;
    int curr_iter;
    my_m1 n_d1;
    my_m2 n_d2;

    /* for each segment list length */
    for (short iter = maxiter ; iter > 0 ; iter--) {
        /* for each destination node */
        for (short v = 0 ; v < sr->nbNode ; v++) {
            if (v == src) {
                continue;
            }
            /*for each available distance */
            for (my_m1 dist = 0 ; dist < d[iter][v].size ; dist++) {
                if (d[iter][v].paths[dist] == INF) {
                    continue;
                }

                /* if seglist already computed, skip */
                if (sl[iter][v][dist].size) {
                    continue;
                }

                /* init retreive loop */
                pred = v;
                n_d1 = dist;
                n_d2 = d[iter][v].paths[dist];
                curr_iter = iter;
                stacksize = 0;
                memset(stack, 0, 10 * sizeof(struct segment_list*));
                short p_pred = 0;
                my_m2 p_n_d1 = 0;
                my_m2 p_n_d2 = 0;

                while (pred != src) {
                    if (!sl[curr_iter][pred][n_d1].size) {
                        stack[stacksize++] = &sl[curr_iter][pred][n_d1];
                    }
                    p_pred = d[curr_iter][pred].preds[n_d1];
                    curr_iter--;

                    for (Edge_t* l = sr->pred[pred][p_pred] ; l ; l = l->next) {
                        p_n_d1 = n_d1 - l->m1;
                        p_n_d2 = n_d2 - l->m2;

                        if (d[curr_iter][p_pred].paths[p_n_d1] != p_n_d2) {
                            continue;
                        }


                        for (int i = 0 ; i < stacksize ; i++) {
                            stack[i]->seg[stack[i]->size++] = pred;
                        }

                        pred = p_pred;
                        n_d1 = p_n_d1;
                        n_d2 = p_n_d2;
                        break;
                    }

                    if (pred != p_pred) {
                        ERROR("Can't find path\n");
                        break;
                    }
                }
            }
        }
    }

    return sl;
}


void print_segment_list(struct segment_list*** sl, int maxiter, int nbNodes)
{
    for (int iter = 0 ; iter <= maxiter ; iter++) {
        for (int v = 0 ; v < nbNodes ; v++) {
            INFO("For Node %d at iter %d:\n", v, iter);
            for (int dist = 0 ; dist < 1000 ; dist++) {
                if (sl[iter][v][dist].size) {
                    printf("Size %d : ", sl[iter][v][dist].size);
                    for (int i = 0 ; i < sl[iter][v][dist].size ; i++) {
                        printf(" %d ", sl[iter][v][dist].seg[i]);
                    }
                    printf("\n");
                }
            }
        }
    }
}