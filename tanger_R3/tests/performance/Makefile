LEVEL=../..

DIRS=bank-ab counter counter-ab fairness-ab linkedlist linkedlist-ab rbtree rbtree-ab

include $(LEVEL)/Makefile.common

.PHONY: fakecheck
fakecheck:
	counter/counter
	linkedlist-ab/intset -n 2
	rbtree-ab/intset -n 2