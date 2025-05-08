 == SHORT DESCRIPTION ==

Simple FUSE read only driver to access data from (not only) game archives.

This never was intended as real tool. I've written this for entertainment
only. And if I will able to revese engineer file format to that extent, that
I can access data from it. As it now supports dozens of different archive
types, I decided to publish it - maybe it will be useful for someone.

 == USAGE ==

Run ./gamefs without parameters to get list of suported archives.

To mount one of them run:
./gamefs --game archive_type --file path_to_archive_file mount_point


 == WRITE SUPPORT ==

This will be never added. It is almost impossible to create smiple universal
tool with write support. First of all, archives are very different of each
other, and will be hard to generalize code. Second, it requires full
understanding of it format. For reading you can skip some metadata not
required for it. When you change/add something you must provide them, as game
may not recognize them as correct, and crash or refuse to run. And third, most
of this file formats was not crated with easy modification in mind. Adding one
new file o extned existing one by one byte often may reqire creating entire
archive from beginning.


 == ADDING NEW ARCHIVE ==

If you want add new archive type, and you don't know where to begin. Just look
at modules source code. Most of them have less than 100 LOC. Simply, find
simmilar one, and change it for your need.

Main idea is to create tree structure corresponding to the archive, and pass
it to FUSE. Each node of this tree can be directory, regular file, or packed
file (zlib). In case of files you must specify also it size and offset inside
of archive. In case of packed one - also packed size. Also in case if some of
your files are packed, you should change data access method from file directly
to memory one. This allow to unpack it, and present original data to the user.

You need implement only two functions. One for creating tree of files, which is
required. And optional (can return false) for autodetecting if specified file
is in supported format.

To add files to tree you can use following functions:
    struct filenode * generic_add_file(struct filenode *node, const char *file, unsigned flags)
    It adds 'file' of type specified in 'flags' to the tree 'node'.

    struct filenode * generic_add_path(struct filenode *node, const char *path, unsigned flags)
    It adds file pointed by 'path' of type specified in 'flags' to the tree
    'node' along with all directories specified.

    struct filenode * generic_add_unknown(unsigned flags)
    It add nameless byte stream of type specified in 'flags' as file with
    default name to the 'lost-found' directory.

Last you need make your new module availabe to the main program. You can do
that by adding externs to your functions int "modules.h" header file. And
by adding new entry to the 'struct gametable' in 'gamefs.c". It has format:
    { "module_name", "module description", creating_tree_func, autodetect_func }

Now recompile, and new module should be available from now.


 == FUTURE DEVELOPMENT ==

I've no intention to do much in this project. For time to time I may add some
new archive for entertainment like i did before. But not expect more than couple
archives a year (optimistically). And especially not for your favorite game,
which file archives I never analize, and even seen.


 == OTHER ==

Note that some of supported archives was not design to keep files in it, but
only generic bit streams. In those cases file names showed after mounting may
have some default names or in best case stream signatures.

Consult BUGS a TODO files for known limitations.
