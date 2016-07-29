/**
 * Created by peanut on 14/01/16.
 */

import static org.junit.Assert.*;

import org.junit.*;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.*;

@RunWith(Parameterized.class)
public class ReferenceCompareTest {
    String file = null;
    @Parameterized.Parameters( name = "{index}: {0}" )
    public static Collection<Object[]> data() {
        File testFolder = new File("../tests");
        //ArrayList<File> files = new ArrayList<>();
        Collection<Object[]> params = new ArrayList<>();
        for (File file:testFolder.listFiles()){
            Object[] param = new Object[1];
            if (file.getName().endsWith(".clj")) {
                param[0] = file.getPath();
                params.add(param);
            }
        }
        return params;
    }

    public ReferenceCompareTest(String file){
        this.file = file;
    }


    @BeforeClass public static void setup() throws IOException{
        executeCommand("chmod u+x setup.sh && ./setup.sh");
    }

    @AfterClass public static void tearDown(){
        //System.out.println("travis_fold:end:tests");
    }

    @Test public void anotherSample() throws IOException{
        String file = this.file;
        //String reference = getReferenceOutput("../tests/closure_test.clj");
        //String clojit = getClojitOutput("../tests/closure_test.clj");
        String reference = getReferenceOutput(file);
        String clojit = getClojitOutput(file);
        assertEquals(reference,clojit);
    }

    private String getClojitOutput(String sFilename) throws IOException{
        executeCommand("java -jar ./lib/clojit-0.1.0-SNAPSHOT-standalone.jar " + sFilename);
        return executeCommand("../main " + sFilename.replace(".clj",".cvmb"));
    }

    private  String getReferenceOutput(String sFilename) throws IOException{
        return executeCommand("java -cp ./lib/clojure-1.7.0.jar clojure.main " + sFilename);
    }

    private static String executeCommand(String sCommand) throws IOException{

        Runtime rt = Runtime.getRuntime();
        String[] commands = {"sh","-c",sCommand};
        Process proc = rt.exec(commands);

        BufferedReader stdInput = new BufferedReader(new
                InputStreamReader(proc.getInputStream()));

        BufferedReader stdError = new BufferedReader(new
                InputStreamReader(proc.getErrorStream()));

        // read the output from the command
        System.out.println("Here is the standard output of the command:\n");
        StringBuilder sb = new StringBuilder();
        String s = null;
        while ((s = stdInput.readLine()) != null) {
            //System.out.println(s);
            sb.append(s);
        }

        // read any errors from the attempted command
        /*System.out.println("Here is the standard error of the command (if any):\n");
        while ((s = stdError.readLine()) != null) {
            //System.err.println(s);
            sb.append(s);
        }*/
        return sb.toString();
    }
}

