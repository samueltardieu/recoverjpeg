dist_man_MANS = recoverjpeg.1 recovermov.1 sort-pictures.1 remove-duplicates.1
extra_DIST = recoverjpeg.md recovermov.md sort-pictures.md remove-duplicates.md

.md.1:
	pandoc -s -w man $< -o $@
