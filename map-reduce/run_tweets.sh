hadoop com.sun.tools.javac.Main WordCount.java
jar -cvf WordCount.jar WordCount*.class
hadoop fs -rm -r /user/root/output
hadoop jar WordCount.jar WordCount /user/root/data /user/root/output