#!/bin/sh
echo "Sanity test of pn_shm_tool."

# Create a shared memory area for test
f="shm_test_2"
echo "Creating /dev/shm/$f for test"

$(./pn_shm_tool -c "$f" -s 4)


# 1 Verify correct size of data

data_in="abcd"
echo "Test data: $data_in"

$(echo -n -e "$data_in" | ./pn_shm_tool -w "$f")
data_out=$(./pn_shm_tool -r "$f")

echo "Data out:  $data_out"

if [ "$data_out" != "$data_in" ]; then
  echo "Error"
  exit 1
fi


# 2 Verify correct too small input data

data_in="ABC"
echo "Test data: $data_in"

$(echo -n -e "$data_in" | ./pn_shm_tool -w "$f")
data_out=$(./pn_shm_tool -r "$f")

echo "Data out:  $data_out"

# First three bytes changed. Last byte keeps value
if [ "$data_out" != "ABCd" ]; then
  echo "Error"
  exit 1
fi


# 3 Verify correct too large input data

data_in="123456"
echo "Test data: $data_in"

$(echo -n -e "$data_in" | ./pn_shm_tool -w "$f")
data_out=$(./pn_shm_tool -r "$f")

echo "Data out:  $data_out"

# All bytes changed. Size unchanged
if [ "$data_out" != "1234" ]; then
  echo "Error"
  exit 1
fi


# 4 faulty parameter

echo "Testing invalid parameter:"
show_cmd=$(./pn_shm_tool -d "$f")
ret_val=$?
echo " "

if [ $ret_val -ne 1 ]; then
  echo "Error"
  exit 1
fi

# 5 Verify reading bit

data_in="1234"
echo "Test data: $data_in"
echo "The stored data as ASCII: 0x31 0x32 0x33 0x34"

$(echo -n -e "$data_in" | ./pn_shm_tool -w "$f")

data_out=$(./pn_shm_tool -b "$f" -n 0)
ret_val=$?
if [ $ret_val -ne 0 ]; then
  echo "Error when reading 0: Wrong exit code"
  exit 1
fi
if [ "$data_out" != "1" ]; then
  echo "Error reading bit 0: Wrong value"
  exit 1
fi

data_out=$(./pn_shm_tool -b "$f" -n 1)
if [ "$data_out" != "0" ]; then
  echo "Error reading bit 1"
  exit 1
fi

data_out=$(./pn_shm_tool -b "$f" -n 2)
if [ "$data_out" != "0" ]; then
  echo "Error reading bit 2"
  exit 1
fi

data_out=$(./pn_shm_tool -b "$f" -n 3)
if [ "$data_out" != "0" ]; then
  echo "Error reading bit 3"
  exit 1
fi


# 6 faulty parameter to read bit
echo "Testing invalid parameter:"
data_out=$(./pn_shm_tool -b "$f" -n 32)
ret_val=$?
echo " "

if [ $ret_val -ne 1 ]; then
  echo "Error"
  exit 1
fi

echo "OK, test passed"
