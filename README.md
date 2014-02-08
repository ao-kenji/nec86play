nec86play
=========

nec86play - PC-9801-86 sound board test program for OpenBSD/luna88k

This is my experimental program to use PC-9801-86 sound board on
OpenBSD/luna88k.

To use this program, you need to apply the following patch.  Then
rebuild your kernel, install, and reboot.
```
Index: sys/arch/m88k/m88k/mem.c
===================================================================
RCS file: /cvs/src/sys/arch/m88k/m88k/mem.c,v
retrieving revision 1.1
diff -u -r1.1 mem.c
--- sys/arch/m88k/m88k/mem.c	31 Dec 2010 21:38:08 -0000	1.1
+++ sys/arch/m88k/m88k/mem.c	8 Feb 2014 07:50:47 -0000
@@ -168,7 +168,13 @@
         off_t off;
 	int prot;
 {
-	return (-1);
+	/* XXX: temporary hack to mmap PC-9801 extention boards */
+	switch (minor(dev)) {
+	case 0:
+		return off;
+	default:
+		return (-1);
+	}
 }
 
 /*ARGSUSED*/


```
