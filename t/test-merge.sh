#!/bin/sh

# This script tests that merging 2 mtbl files together using mtbl_merge produces
# the correct result.  It exercises the block size, compression type/level, and
# thread count CLI options.

if [ -z "${top_srcdir}" ]; then
    echo "top_srcdir variable not set"
    exit 1
fi

if [ -z "${top_builddir}" ]; then
    echo "top_builddir variable not set"
    exit 1
fi

ulimit -c 0

print_result_of_last_cmd()
{
	if [ "$?" -eq 0 ] ; then
		echo "PASS"
	else
		echo "FAIL"
		exitcode=1
	fi
}

# Will be set to 1 if any test within this script fails.
exitcode=0

# Set up mtbl_merge with our test DSO.
mtbl_merge="$top_builddir/src/mtbl_merge"
export MTBL_MERGE_DSO="$top_builddir/t/test-merge-func.so"
export MTBL_MERGE_FUNC_PREFIX="test_merge"

# full_data is the result of merging the 2 half_datas.
full_data="$top_srcdir/t/test-merge-full.data"
half_data0="$top_srcdir/t/test-merge-half0.data"
half_data1="$top_srcdir/t/test-merge-half1.data"

# Output of test merges will be written here.
tmp_data="$top_srcdir/t/test-merge-tmp.data"
rm -f $tmp_data

#===============#
# Begin testing #
#===============#
echo -n "Verify mtbl_merge with no extra args produces correct output: "
$mtbl_merge $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
cmp -s $tmp_data $full_data
print_result_of_last_cmd
rm -f $tmp_data

#=================#
# Test block size #
#=================#
echo -n "Verify mtbl_merge with block size = 1024 produces correct output: "
$mtbl_merge -b 1024 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
entry_count=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 8 | tail -n 1)
block_size=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $4}' | head -n 6 | tail -n 1)
[ "$entry_count" -eq 90 -a "$block_size" = "1,024" ]
print_result_of_last_cmd
rm -f $tmp_data

echo -n "Verify mtbl_merge with block size = 65536 produces correct output: "
$mtbl_merge -b 65536 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
entry_count=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 8 | tail -n 1)
block_size=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $4}' | head -n 6 | tail -n 1)
[ "$entry_count" -eq 90 -a "$block_size" = "65,536" ]
print_result_of_last_cmd
rm -f $tmp_data

#==================#
# Test compression #
#==================#
for comp_type in	\
	zlib		\
	zstd		\
	snappy		\
	lz4		\
	lz4hc		\
	none		\
; do
	# Do a pass using the default compression level.
	echo -n "Verify mtbl_merge with compression = $comp_type produces correct output: "
	$mtbl_merge -c $comp_type $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
	entry_count=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 8 | tail -n 1)
	compression_type=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 11 | tail -n 1)
	[ "$entry_count" -eq 90 -a "$compression_type" = "$comp_type" ]
	print_result_of_last_cmd
	rm -f $tmp_data

	# If the current compression type supports it, compare the default compression
	# with a higher compression level, and ensure output file size has decreased.
	if [ "$comp_type" = "zlib" ] || [ "$comp_type" = "lz4hc" ] || [ "$comp_type" = "zstd" ]; then
		echo -n "Verify higher mtbl_merge compression levels result in higher compactness: "

		# Check the file size with compression level 0.
		$mtbl_merge -c $comp_type -l 0 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
		bigger_file_size=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 2 | tail -n 1)
		rm -f $tmp_data

		# Check the file size with compression level 10.
		$mtbl_merge -c $comp_type -l 10 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
		smaller_file_size=$($top_srcdir/src/mtbl_info $tmp_data | awk '{print $3}' | head -n 2 | tail -n 1)
		rm -f $tmp_data

		# Strip commas off of the file sizes so we can compare them.
		size_bigger=$(echo $bigger_file_size | sed 's/,//')
		size_smaller=$(echo $smaller_file_size | sed 's/,//')
		[ "$size_bigger" -gt "$size_smaller" ]
		print_result_of_last_cmd
	fi
done

#=====================#
# Test multithreading #
#=====================#
echo -n "Verify mtbl_merge with thread count = 16 produces correct output: "
$mtbl_merge -t 16 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
cmp -s $tmp_data $full_data
print_result_of_last_cmd
rm -f $tmp_data

echo -n "Verify mtbl_merge with thread count = 0 produces correct output: "
$mtbl_merge -t 0 $half_data0 $half_data1 $tmp_data > /dev/null 2>&1
cmp -s $tmp_data $full_data
print_result_of_last_cmd
rm -f $tmp_data

return $exitcode
