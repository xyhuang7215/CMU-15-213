all: test-p1 test-p2 test-p3 test-p4 clean

test-p1:
	./hex2raw < answers/p1.txt > tmp.txt
	./ctarget -q -i tmp.txt

test-p2:
	./hex2raw < answers/p2.txt > tmp.txt
	./ctarget -q -i tmp.txt

test-p3:
	./hex2raw < answers/p3.txt > tmp.txt
	./ctarget -q -i tmp.txt

test-p4:
	./hex2raw < answers/p4.txt > tmp.txt
	./rtarget -q -i tmp.txt

clean:
	rm -rf *.o
	rm -f *.txt
