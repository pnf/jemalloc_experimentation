package jemalloctest;

public class JemallocIntegration {
    static {
        System.loadLibrary("native");
    }

    public static void main(String[] args) {
        System.out.println("hihi");
        new JemallocIntegration().sayHello();
    }

    // Declare a native method sayHello() that receives no arguments and returns void
    private native void sayHello();
}
