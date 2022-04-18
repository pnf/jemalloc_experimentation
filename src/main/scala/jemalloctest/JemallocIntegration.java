package jemalloctest;

public class JemallocIntegration {
    static {
        System.out.println("Loading!@@");
        System.loadLibrary("native");
    }

    public static void main(String[] args) {
        System.out.println("hihi");
        JemallocIntegration.sayHello();
    }

    // Declare a native method sayHello() that receives no arguments and returns void
    private static native void sayHello();

    public static native long setup(long config);
    public static native long mkConfig();
    public static native void tearDown();
    public static native void setSleep(long t, long s);
    public static native long alloc(long sz);
    public static native void free(long ptr);

}
