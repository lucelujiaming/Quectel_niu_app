#ifndef _COM_LIST_H_
#define _COM_LIST_H_


#ifndef prefetch
#define prefetch(x) (void)(x)
#endif

#ifndef NULL
#define NULL	0
#endif

#ifdef __compiler_offsetof
#define com_offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define com_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


#define com_container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - com_offsetof(type,member) );})


/* Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct com_list_head_struct {
	struct com_list_head_struct *next, *prev;
} com_list_head_t;

#define COM_LIST_HEAD_INIT(name) { &(name), &(name) }

#define COM_LIST_HEAD(name) \
	struct com_list_head_struct name = COM_LIST_HEAD_INIT(name)

static inline void COM_INIT_LIST_HEAD(struct com_list_head_struct *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __com_list_add(struct com_list_head_struct *new,
			      struct com_list_head_struct *prev,
			      struct com_list_head_struct *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * com_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void com_list_add(struct com_list_head_struct *new, struct com_list_head_struct *head)
{
	__com_list_add(new, head, head->next);
}


/**
 * com_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void com_list_add_tail(struct com_list_head_struct *new, struct com_list_head_struct *head)
{
	__com_list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __com_list_del(struct com_list_head_struct * prev, struct com_list_head_struct * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * com_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: com_list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void com_list_del(struct com_list_head_struct *entry)
{
	__com_list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * com_list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void com_list_replace(struct com_list_head_struct *old,
				struct com_list_head_struct *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void com_list_replace_init(struct com_list_head_struct *old,
					struct com_list_head_struct *new)
{
	com_list_replace(old, new);
	COM_INIT_LIST_HEAD(old);
}

/**
 * com_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void com_list_del_init(struct com_list_head_struct *entry)
{
	__com_list_del(entry->prev, entry->next);
	COM_INIT_LIST_HEAD(entry);
}

/**
 * com_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void com_list_move(struct com_list_head_struct *list, struct com_list_head_struct *head)
{
	__com_list_del(list->prev, list->next);
	com_list_add(list, head);
}

/**
 * com_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void com_list_move_tail(struct com_list_head_struct *list,
				  struct com_list_head_struct *head)
{
	__com_list_del(list->prev, list->next);
	com_list_add_tail(list, head);
}

/**
 * com_list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int com_list_is_last(const struct com_list_head_struct *list,
				const struct com_list_head_struct *head)
{
	return list->next == head;
}

/**
 * com_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int com_list_empty(const struct com_list_head_struct *head)
{
	return head->next == head;
}

/**
 * com_list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using com_list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is com_list_del_init(). Eg. it cannot be used
 * if another CPU could re-com_list_add() it.
 */
static inline int com_list_empty_careful(const struct com_list_head_struct *head)
{
	struct com_list_head_struct *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * com_list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int com_list_is_singular(const struct com_list_head_struct *head)
{
	return !com_list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct com_list_head_struct *list,
		struct com_list_head_struct *head, struct com_list_head_struct *entry)
{
	struct com_list_head_struct *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void list_cut_position(struct com_list_head_struct *list,
		struct com_list_head_struct *head, struct com_list_head_struct *entry)
{
	if (com_list_empty(head))
		return;
	if (com_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		COM_INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __com_list_splice(const struct com_list_head_struct *list,
				 struct com_list_head_struct *prev,
				 struct com_list_head_struct *next)
{
	struct com_list_head_struct *first = list->next;
	struct com_list_head_struct *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * com_list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void com_list_splice(const struct com_list_head_struct *list,
				struct com_list_head_struct *head)
{
	if (!com_list_empty(list))
		__com_list_splice(list, head, head->next);
}

/**
 * com_list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void com_list_splice_tail(struct com_list_head_struct *list,
				struct com_list_head_struct *head)
{
	if (!com_list_empty(list))
		__com_list_splice(list, head->prev, head);
}

/**
 * com_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void com_list_splice_init(struct com_list_head_struct *list,
				    struct com_list_head_struct *head)
{
	if (!com_list_empty(list)) {
		__com_list_splice(list, head, head->next);
		COM_INIT_LIST_HEAD(list);
	}
}

/**
 * com_list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void com_list_splice_tail_init(struct com_list_head_struct *list,
					 struct com_list_head_struct *head)
{
	if (!com_list_empty(list)) {
		__com_list_splice(list, head->prev, head);
		COM_INIT_LIST_HEAD(list);
	}
}

/**
 * com_list_entry - get the struct for this entry
 * @ptr:	the &struct com_list_head_struct pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define com_list_entry(ptr, type, member) \
	com_container_of(ptr, type, member)

/**
 * com_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define com_list_first_entry(ptr, type, member) \
	com_list_entry((ptr)->next, type, member)

/**
 * com_list_for_each	-	iterate over a list
 * @pos:	the &struct com_list_head_struct to use as a loop cursor.
 * @head:	the head for your list.
 */
#define com_list_for_each(pos, head) \
	for (pos = (head)->next; prefetch(pos->next), pos != (head); \
        	pos = pos->next)

/**
 * __com_list_for_each	-	iterate over a list
 * @pos:	the &struct com_list_head_struct to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from com_list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __com_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * com_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct com_list_head_struct to use as a loop cursor.
 * @head:	the head for your list.
 */
#define com_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

/**
 * com_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct com_list_head_struct to use as a loop cursor.
 * @n:		another &struct com_list_head_struct to use as temporary storage
 * @head:	the head for your list.
 */
#define com_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * com_list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct com_list_head_struct to use as a loop cursor.
 * @n:		another &struct com_list_head_struct to use as temporary storage
 * @head:	the head for your list.
 */
#define com_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     prefetch(pos->prev), pos != (head); \
	     pos = n, n = pos->prev)

/**
 * com_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define com_list_for_each_entry(pos, head, member)				\
	for (pos = com_list_entry((head)->next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head); 	\
	     pos = com_list_entry(pos->member.next, typeof(*pos), member))

/**
 * com_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define com_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = com_list_entry((head)->prev, typeof(*pos), member);	\
	     prefetch(pos->member.prev), &pos->member != (head); 	\
	     pos = com_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use in com_list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in com_list_for_each_entry_continue().
 */
#define list_prepare_entry(pos, head, member) \
	((pos) ? : com_list_entry(head, typeof(*pos), member))

/**
 * com_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define com_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = com_list_entry(pos->member.next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head);	\
	     pos = com_list_entry(pos->member.next, typeof(*pos), member))

/**
 * com_list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define com_list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = com_list_entry(pos->member.prev, typeof(*pos), member);	\
	     prefetch(pos->member.prev), &pos->member != (head);	\
	     pos = com_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * com_list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define com_list_for_each_entry_from(pos, head, member) 			\
	for (; prefetch(pos->member.next), &pos->member != (head);	\
	     pos = com_list_entry(pos->member.next, typeof(*pos), member))

/**
 * com_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define com_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = com_list_entry((head)->next, typeof(*pos), member),	\
		n = com_list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = com_list_entry(n->member.next, typeof(*n), member))

/**
 * com_list_for_each_entry_safe_continue
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define com_list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = com_list_entry(pos->member.next, typeof(*pos), member), 		\
		n = com_list_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = com_list_entry(n->member.next, typeof(*n), member))

/**
 * com_list_for_each_entry_safe_from
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define com_list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = com_list_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = com_list_entry(n->member.next, typeof(*n), member))

/**
 * com_list_for_each_entry_safe_reverse
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define com_list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = com_list_entry((head)->prev, typeof(*pos), member),	\
		n = com_list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = com_list_entry(n->member.prev, typeof(*n), member))

static inline int com_list_size(struct com_list_head_struct *ll)
{
    int size = 0;
    com_list_head_t *pos;
    com_list_for_each(pos, ll) {
        size++;
    }
    return size;
}
#endif
