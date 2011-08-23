/*
 * This is a very simple program that will attempt to do the following:
 *    lock FILE
 *    sleep SLEEPTIME
 *    unlock FILE
 *
 * This was originally used by a scqad test to test lock contention.  
 * The first use case is to track down a ceph bug:
 *   - NODE 1 : sclockandhold /fsscale0/sclockandhold.test 10 (PID1)
 *   - LEADER : sleep 2
 *   - NODE 2 : sclockandhold /fsscale0/sclockandhold.test 10 (PID2)
 *   - LEADER : kill PID2
 *   - NODE 1 : after unlocking and trying to get 2nd lock, will hang*
 *   
 *   *Previously this test would attempt to relock and unlock the file
 *    after the sleep period
 *
 * 
 * Usage
 *    sclockandhold FILE SLEEPTIME [OFFSET LENGTH]
 *    Where FILE is the file to lock, SLEEPTIME is how long to hold
 *    the lock, and OFFSET and LENGTH are optional parameters
 *    specifying the offset to lock and the length of the lock. These
 *    default to 0 and 1; if you want to specify the length you
 *    must specify the offset.
 */
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

using namespace std;

int lockFile(int fd, int l_start, int l_len)
{
    struct flock lock;
    
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = l_start;
    lock.l_len = l_len;
    lock.l_pid = 0;

    cout << "Trying to get lock" << endl;
    if (fcntl(fd, F_SETLKW, &lock) == 0)
    {
        cout << "Got lock" << endl;
        return 0;
    }
    else
    {
        cerr << "Failed to get lock (" << strerror(errno) << ")" << endl;
        return 1;
    }
    return 1;
}

void unlockFile(int fd)
{
    struct flock lock;
    
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 1;
    lock.l_pid = 0;

    cout << "Unlocking" << endl;
    if (fcntl(fd, F_SETLKW, &lock) == 0)
    {
        cout << "unlocked" << endl;
    }
    else
    {
        cerr << "Failed to unlock (" << strerror(errno) << ")" << endl;
    }
}

int createFile(const char *fileName)
{
    int fd = open(fileName, O_CREAT|O_RDWR, 0600);
    if (fd <= 0)
    {
        cerr << "Unable to open " << fileName << " (" << 
            strerror(errno) << ")." << endl;
        return -1;
    }
    if (ftruncate(fd, 2) != 0)
    {
        cerr << "Unable to truncate " << fileName << " (" << 
            strerror(errno) << ")." << endl;
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 5)
    {
        cerr << "Usage: " << argv[0] << " <path> <lock hold time>" << endl;
        return 1;
    }
    unsigned int holdTime = strtoul(argv[2], NULL, 0);
    unsigned int offset = 0;
    unsigned int length = 1;

    if (argc > 3) {
      offset = strtoul(argv[3], NULL, 0);
      if (argc > 4) {
	length = strtoul(argv[4], NULL, 0);
      }
    }

    int fd;
    if ((fd = createFile(argv[1])) <= 0)
    {
        return 1;
    }

    if (!lockFile(fd, offset, length)) {
		cout << "Holding lock for " << holdTime << " seconds." << endl;
		sleep(holdTime);
		unlockFile(fd);
		return 0;
    } else {
    	return 1;
    }

    return 1;
}
