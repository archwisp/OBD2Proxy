#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <VersionCheck.h>

// Stolen from https://www.geeksforgeeks.org/compare-two-version-numbers/

// Method to compare two versions
// Does not work with leading zeros 
// Returns -1 if v2 is smaller
// Returns 1 if v1 is smaller
// Returns 0 if equal 

int compareVersions(const char* v1, const char* v2) {
   // vnum stores each numeric 
    // part of version 
    int vnum1 = 0, vnum2 = 0; 

    // loop until both string are 
    // processed 
    for (int i = 0, j = 0; (i < strlen(v1)
                            || j < strlen(v2));) { 
        // storing numeric part of 
        // version 1 in vnum1 
        while (i < strlen(v1) && v1[i] != '.') { 
            vnum1 = vnum1 * 10 + (v1[i] - '0'); 
            i++; 
        } 

        // storing numeric part of 
        // version 2 in vnum2 
        while (j < strlen(v2) && v2[j] != '.') { 
            vnum2 = vnum2 * 10 + (v2[j] - '0'); 
            j++; 
        } 

        if (vnum1 > vnum2) 
            return -1; 
        if (vnum2 > vnum1) 
            return 1; 

        // if equal, reset variables and 
        // go for next numeric part 
        vnum1 = vnum2 = 0; 
        i++; 
        j++; 
    } 
    return 0; 
}

void testCompareVersions() {
    assert(compareVersions("0.0.1", "0.0.1") == 0);
    assert(compareVersions("0.0.1", "0.0.2") == 1);
    assert(compareVersions("0.0.2", "0.0.1") == -1);
    

    assert(compareVersions("0.0.1", "0.0.2") == 1);
    assert(compareVersions("0.0.1", "0.0.2") == 1);
    assert(compareVersions("0.0.1", "0.0.5") == 1);
    assert(compareVersions("0.0.1", "0.0.9") == 1);
    assert(compareVersions("0.0.1", "0.0.10") == 1);
    assert(compareVersions("0.0.1", "0.0.15") == 1);
    assert(compareVersions("0.0.1", "0.0.99") == 1);
    assert(compareVersions("0.0.99", "0.1.0") == 1);
    assert(compareVersions("0.1.53", "0.1.75") == 1);
    assert(compareVersions("21.22.23", "21.22.24") == 1);
    assert(compareVersions("21.20.24", "21.22.24") == 1);
    assert(compareVersions("0.1.4", "0.1.5") == 1);
    assert(compareVersions("0.2.1", "0.2.8") == 1);
}
