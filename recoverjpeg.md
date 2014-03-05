% RECOVERJPEG(1) Recoverjpeg User Manuals
% Samuel Tardieu <sam@rfc1149.net>
% April 25, 2013

# NAME
recoverjpeg - recover jpeg pictures from a filesystem image

# SYNOPSIS

recoverjpeg [*options*] *device*

# DESCRIPTION

Recoverjpeg tries to identify jpeg pictures from a filesystem image. To
achieve this goal, it scans the filesystem image and looks for a jpeg
structure at blocks starting at 512 bytes boundaries.

Salvaged jpeg pictures are stored by default under the name
*imageXXXXX.jpg* where *XXXXX* is a five digit number starting at
zero. If there are more than 100,000 recovered pictures, recoverjpeg
will start using six figures numbers and more as soon as needed, but
the 100,000 first ones will use a five figures number. Options *-f* and
*-i* can override this behaviour.

Note that *device* is not necessarily a physical device. It may also be
a file containing a copy of the faulty device in order to reduce the
actual processing time and the stress imposed to an already defective
hardware. `dd`(1) or `ddrescue`(1) may be used to create such a working
copy.

# OPTIONS

-h
: Display an help message.

-b *blocksize*
: Set the size of blocks in bytes. On most file systems, setting it to
512 (the default) will work fine as any large file will be stored on
512 bytes boundaries. Setting it to 1 maximize the chances of finding
very small files if the filesystems aggregates them (UFS for example)
at the expense of a much longer running time.

-d *formatstring*
: Set the directory format string (printf-style, default: use the current
directory). When used, 0 will be used for the 100 first images, 1 for
the 100 next images, and so on. The goal of this option is to circumvent
the directory size limit imposed by some file systems.

-f *formatstring*
: Set the file name format string (printf-style, default:
"image%05d.jpg"). It is used with the image index as an integer argument.

-i *integerindex*
: Set the initial index value for image numbering (default: 0).

-m *maxsize*
: Maximum size of extract jpeg files. If a file would be larger than that,
it is discarded. The default is 6 MiB.

-q
: Be quiet and do not display anything.

-r *readsize*
: Set the readsize in bytes. By default, this is 128 MiB. Using a large
readsize reduces the number of system calls but consumes more memory. The
readsize will automatically be adjusted to be a multiple of the system
page size. It **must** be greater than the *maxsize* parameter.

-s *cutoffsize*
: Set the cutoff size in bytes. Files smaller than that will be ignored.

-v
: Be verbose and describes the process of jpeg identification. By default,
if this flag is not used, recoverjpeg will print a progress bar showing
how much it has analyzed already and how many jpeg pictures have been
recovered.

-V
: Display program version and exit.

All the sizes may be suffixed by a *k*, *m*, *g*, or *t* letter to
indicate KiB, MiB, GiB, or TiB. For example, 6m correspond to 6 MiB (6291456
bytes).

# EXAMPLES

Recover as many pictures as possible from the memory card located in
`/dev/sdc`:

    recoverjpeg /dev/sdc

Do the same thing but ignore files smaller than one megabyte:

    recoverjpeg -s 1m /dev/sdc

Recover as many pictures as possible from a crashed ReiserFS file
system (which does not necessarily store pictures at block boundaries)
in `/dev/sdb1`:

    recoverjpeg -b 1 /dev/sdb1

Do the same thing in a memory constrained environment where no more than
16MB of RAM can be used for the operation:

    recoverjpeg -b 1 -r 16m /dev/sdb1

# COPYRIGHT

Copyright (c) 2004-2013 Samuel Tardieu <sam@rfc1149.net>.
This is free software; see the source for copying conditions. There is
NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

If recoverjpeg saves your day and you liked it, you are welcome to send
me the best rescued ones by email (please send only 800x600 versions
of the pictures) and authorize me to put them online (indicate which
contact information you want me to use for credits).

# SEE ALSO

`recovermov`(1) `sort-pictures`(1) `remove-duplicates`(1)

# KNOWN BUGS

Recoverjpeg does not include a complete jpeg parser. You may need to use
sort-pictures afterwards to identify bogus pictures. Some pictures may be
corrupted but have a correct structure; in this case, the image may be
garbled. There is no automated way to detect those pictures with a 100%
success rate.
