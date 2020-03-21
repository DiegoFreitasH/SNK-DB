/*
 * Least Recently Used (LRU)
 *
 * Use this algorithm to develop new page replacement policies.
 *
 */
#ifndef POLICY_H_INCLUDED
#define POLICY_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include "../db_buffer.h"
#include "../../db_config.h"

#define MIN_LC BUFFER_SIZE*0.1

void insert_MRU(struct List * list, struct Node * node);
struct Node * remove_LRU(struct List * list);
void move_to_MRU(struct List * list, struct Page * page);
struct Node * clock_police_victim(struct List * list);
struct Node * select_victim(struct List * list);
void move_to_hot_MRU(struct List * hot, struct List * cold, struct Page * page);


struct List * cold;
struct List * hot;

/*
 * This function is called after initializing the buffer and before page requests.
 * 
 * Initialize the structures used in the page replacement policy here.
 */
void buffer_policy_start(){
    printf("\nBuffer Replacement Policy: %s", __FILE__);
    printf("\n---------------------------------------------------------------------------------------------------");
	
    cold = list_create(buffer_print_page, NULL);
	hot = list_create(buffer_print_page, NULL);
}


struct Page * buffer_request_page(int file_id, long block_id, char operation){

	// It is mandatory to call this two functions (buffer_find_page, buffer_computes_request_statistics)
	//--------------------------------------------------------
	struct Page * page = buffer_find_page(file_id, block_id);
	buffer_computes_request_statistics(page, operation);
	//--------------------------------------------------------
	
	if(page != NULL){ //HIT - Move page to MRU of Hot list
		
		move_to_hot_MRU(hot, cold, page);
		page->reference = page->reference + 1;

	} else { // MISS - page is not in Buffer (struct Page * page == NULL)
		
		if(buffer_is_full() == FALSE){  // Insert page in MRU of Cold list
			
			page = buffer_get_free_page();
			struct Node * new_node = list_create_node(page);
			buffer_load_page(file_id, block_id, page);
			insert_MRU(cold, new_node);
		
		}
		else { // Needs replacement 
			printf("\n ---- REPLACEMENT ------ ");	
			struct Node * lru_node;
			
			if(cold->size <= MIN_LC){
				lru_node = select_victim(hot);	
			}
			else {
				lru_node = select_victim(cold);
			}

			struct Page * victim = (struct Page *) lru_node->content;

			buffer_flush_page(victim); // Flush the data to the secondary storage media if is dirty

			page = buffer_reset_page(victim); // To avoid malloc a new page we reuse the victim page

			buffer_load_page(file_id, block_id, page); // Read new data from storage media
			
			insert_MRU(cold, lru_node);
		
		}
		page->reference = 0;
	}
	set_dirty(page, operation);
	return page;
}

void insert_MRU(struct List * list, struct Node * node){
	list_insert_node_head(list,node);
	((struct Page*)node->content)->extended_attributes = node;
}

struct  Node * remove_LRU(struct List * list){
	return list_remove_tail(list);
}

void move_to_MRU(struct List * list, struct Page * page){
	struct Node * node = (struct Node *) page->extended_attributes;
	list_remove(list,node);
	list_insert_node_head(list,node);
}

struct Node * clock_police_victim(struct List * list){ // Executes the clock police
	struct Node * node = list->tail;
	struct Page * page = ((struct Page*)node->content);
	
	while(page->reference != 0){ // Search for a page with reference 0 from LRU to MRU
		page->reference = page->reference - 1;
		node = node->prev;
		if(node == NULL){
			node = list->tail;
		}
		page = ((struct Page*)node->content);
	}

	return node;
}

struct Node * select_victim(struct List * list){
	struct Node * node = list->tail;

	while(node != NULL && ((struct Page*)node->content)->dirty_flag != PAGE_CLEAN ){ // Search for the first Clean Page
		node = node->prev;
	}
	
	if(node == NULL) { // All the pages are dirty
		node = clock_police_victim(list);
	}

	return list_remove(list, node);
}

void move_to_hot_MRU(struct List * hot, struct List * cold, struct Page * page){
	struct Node * node = (struct Node *) page->extended_attributes;
	
	if(node->list == cold) { 
		list_remove(cold, node);
	}
	else { 
		list_remove(hot, node); 
	} 
	
	list_insert_node_head(hot, node);
}

#endif
