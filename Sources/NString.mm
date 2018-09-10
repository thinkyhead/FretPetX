#import "String.h"
#import "Assertions.h"

void String::runUnitTests() {

    String a = "apple";
    String b = a;
    assert(a == b);
    assert(b == "apple");
    
    NSString *c = a + " butter";
    assert(String(c) == "apple butter");
    a += " butter";
    assert( a == String(c) );
    
    String d = "foo %d bar";
    String e = [NSString stringWithFormat:d, 42];
    assert(e == "foo 42 bar");
    
    a = "apple";
    b = "banana";
    assert(a != b);
    assert(a < b);
    assert(not (a >= b));
    assert(b > a);
    assert(not (b <= a));
    
    CFRange r = CFStringFind(b, CFSTR("na"), 0);
    assert(r.location == 2);
    
    String s = a.substr(0,3);
    assert(s == "app");
    s = a.substr(3);
    assert(s == "le");
    s = a.substr(2,3);
    assert(s == "ple");
    
    long pos = a.find("app");
    assert(pos == 0);
    pos = a.find("foo");
    assert(pos == -1);
    pos = b.find("an");
    assert(pos == 1);
    pos = b.rfind("an");
    assert(pos == 3);
    pos = b.rfind("foo");
    assert(pos == -1);
    
    String f = "foo.dir/bar.txt";
    String ext = f.stripExtension();
    assert(f == "foo.dir/bar");
    assert(ext == "txt");
}

void test1s() {
    String a = "app";
    String b = a + "le";
    assert(b == "apple");
    assert(b < "banana");
    b += " pie";
    assert(b != "cherry pie");
}

void test2s() {
    NSString *a = @"app";
    NSString *b = [a stringByAppendingString:@"le"];
    assert([b isEqualToString:@"apple"]);
    assert([b compare:@"banana"] == NSOrderedAscending);
    b = [b stringByAppendingString:@" pie"];
    assert(![b isEqualToString:@"cherry pie"]);
}

