/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   _malloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/24 21:56:50 by fbelthoi          #+#    #+#             */
/*   Updated: 2023/01/01 22:28:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stddef.h>
#include <sys/mman.h>
#include <stdio.h>

t_header *freep = NULL;

static void print_freep() {

	//printf("__print_freep\n");

	t_header *h = freep;
	while (h)
	{
		printf("%p\n", h);
		h = h->next;
		if (h == freep)
			break;
	}

	printf("__\n");
}

static char is_a_zone_header(t_header *hdrp)
{
	if (hdrp == TINY_ZONE || hdrp == SMALL_ZONE || hdrp == LARGE_ZONE)
		return 1;
	return 0;
}

static void	rm_from_free_list(t_header *hdrp) {
	t_header *prev = hdrp->prev;
	t_header *next = hdrp->next;

	next->p_size = prev->size;
	prev->next = next;
	next->prev = prev;

	if (prev == hdrp)
		freep == NULL;

	if (freep == hdrp)
		freep = next;
}

static void	coalesce(t_header *hdrp) {
	t_header *hprev = hdrp->prev;
	t_header *hnext = hdrp->next;

	if (is_a_zone_header(hdrp) == 1)
		return;

	if (is_a_zone_header(hnext) == 0 && (hdrp + BLOCK_SIZE(hdrp)) == hnext) {
		hdrp->size += BLOCK_SIZE(hnext);
		rm_from_free_list(hnext);
	}

	if (is_a_zone_header(hprev) == 0 && hprev == (hdrp - BLOCK_SIZE(hprev)))
	{
		hprev->size += BLOCK_SIZE(hdrp);
		rm_from_free_list(hdrp);
	}
}

static void insert(t_header *hdrp, t_header *prev) {

	t_header *nextp = prev->next;

	prev->next = hdrp;
	hdrp->prev = prev;
	hdrp->p_size = prev->size;

	nextp->prev = hdrp;
	hdrp->next = nextp;
	nextp->p_size = hdrp->size;
}

static void init_freep(t_header *hdrp) {
	freep = hdrp;
	freep->p_size = hdrp->size;
	freep->next = hdrp;
	freep->prev = hdrp;
}

static void	add_to_free_list(t_header *hdrp) {
	t_header *h = freep;

	if (h == NULL)
	{
		init_freep(hdrp);
		return;
	}

	if (hdrp->size > TINY)
	{
		if (hdrp->size < SMALL)
			hdrp = SMALL_ZONE;
		else
			hdrp = LARGE_ZONE;
	}

	while (h->next < hdrp)
	{
		if (h->next == freep)
			break;
		h = h->next;
	}

	insert(hdrp, h);
	coalesce(hdrp);
}

static char *morecore(size_t request) {
	size_t size = ALIGN(OVERHEAD + request);

	t_header *hdrp = mmap(0, CHUNK_ALIGN(size), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (hdrp == (void *) -1)
		return NULL;

	hdrp->size = CHUNK_ALIGN(size) - OVERHEAD;

	add_to_free_list(hdrp);

	return BP(hdrp);
}

static void	init() {
	t_header *tiny_zone;

	tiny_zone = mmap(0, TINY_SPACE + SMALL_SPACE + OVERHEAD, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if (tiny_zone == (void *) -1)
		return;

	tiny_zone->size = 0;

	t_header *tiny_hdrp = (t_header *)((char *)tiny_zone + OVERHEAD);

	tiny_hdrp->size = TINY_SPACE - (OVERHEAD * 2);


	t_header *small_zone = (t_header *)((char *)tiny_zone + TINY_SPACE);

	small_zone->size = 0;

	t_header *small_hdrp = (t_header *)((char *)small_zone + OVERHEAD);

	small_hdrp->size = SMALL_SPACE - (OVERHEAD * 2);


	t_header *large_zone = (t_header *)((char *)small_zone + SMALL_SPACE);

	large_zone->size = 0;

	add_to_free_list(tiny_zone);
	add_to_free_list(tiny_hdrp);
	add_to_free_list(small_zone);
	add_to_free_list(small_hdrp);
	add_to_free_list(large_zone);
}

static char 	*find_in_free_list(size_t n) {
	t_header *h = freep;

	if (h == NULL)
		return NULL;

	if (n > TINY)
	{
		if (n <= SMALL)
			h = SMALL_ZONE;
		else
			h = LARGE_ZONE;
	}

	while (h->size < n) {
		h = h->next;
		if (is_a_zone_header(h))
			return NULL;
	}

	return BP(h);
}

static char		*get_chunk(size_t n) {
	char *bp;
	
	bp = find_in_free_list(n);
	if (bp != NULL)
		return bp;

	bp = morecore(n);

	return bp;
}

static void		part(char *bp, size_t request) {
	t_header *hdrp = HDRP(bp);

	if (hdrp->size <= (request + OVERHEAD))
		return;

	t_header *new = (t_header *)(ALIGN((intptr_t)hdrp + request + OVERHEAD));

	new->size = hdrp->size - (request + OVERHEAD);
	new->p_size = request;
	hdrp->size = request;

	add_to_free_list(new);
	rm_from_free_list(hdrp);
}

void	*_malloc(size_t n) {
	if (freep == NULL)
		init();

	char *bp = get_chunk(n);

	if (bp == NULL)
		return NULL;

	part(bp, n);

	return bp;
}
