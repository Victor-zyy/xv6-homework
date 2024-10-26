	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	/* get current thread stack */	
	movl current_thread, %eax;
	movl %esp, (%eax);
	/* save context */
	pushal;
	/* switch stack */
	movl next_thread, %eax;
	movl (%eax), %eax;
	movl %eax, %esp;
	movl $0x0, next_thread;
	/* restore context */	
	popal;
	/* return to the eip points*/
	ret				/* pop return address from stack */
