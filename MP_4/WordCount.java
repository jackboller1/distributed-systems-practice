import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
import org.apache.hadoop.mapreduce.RecordReader;
import org.apache.hadoop.mapreduce.lib.input.LineRecordReader;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.util.LineReader;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;
import java.io.IOException;

public class WordCount {

    public static class ReadLineMapper extends Mapper<LongWritable, Text, IntWritable, IntWritable> {

		private IntWritable hour = new IntWritable();

		@Override
		protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
			String line = value.toString();
			//split each line by any amount of whitespace
			String[] tweet_tokens = line.split("\\s+");

			if (tweet_tokens.length > 2) {
				String info_type = tweet_tokens[0];

				if ("T".equals(info_type)) {
					String time_str = tweet_tokens[2];
					String[] time_arr = time_str.split(":");
					int hour_of_day = Integer.parseInt(time_arr[0]);
					hour.set(hour_of_day);
					context.write(hour, new IntWritable(1));
				}
			}

		}

    }

    public static class ReadTweetMapper extends Mapper<LongWritable, Text, IntWritable, IntWritable> {

        public int hour_of_day;

        protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
            String[] lines = value.toString().split("\n");
            for (String line : lines) {
                if (line.startsWith("T")) {
                    String[] tweet_tokens = line.trim().split("\\s+");
                    String time_str = tweet_tokens[2];
                    String[] time_arr = time_str.split(":");
                    hour_of_day = Integer.parseInt(time_arr[0]);
                } 
                else if (line.startsWith("W")) {
                    String[] content = line.trim().split("\\s+");
                    for (String word : content) {
                        if (word.toLowerCase().contains("sleep")) {
                            context.write(new IntWritable(hour_of_day), new IntWritable(1));
                            break;
                        }
                    }
                }
            }
        }
        
    }

    public static class PassThroughReducer extends Reducer<IntWritable, IntWritable, IntWritable, IntWritable> {

		@Override
        protected void reduce(IntWritable key, Iterable<IntWritable> values, Context context) throws IOException, InterruptedException {
            int count = 0;
			for (IntWritable value : values) {
				count += value.get();
			}
			context.write(key, new IntWritable(count));
        }
    }


    public static class TweetRecordReader extends RecordReader<LongWritable, Text> {

        private LineRecordReader line_rr;
        private LongWritable currentKey;
        private Text currentValue = new Text();
        private int tweet_length = 4;


        @Override
        public void initialize(InputSplit split, TaskAttemptContext context) throws IOException, InterruptedException {
            line_rr = new LineRecordReader();
            line_rr.initialize(split, context);
        }
        

        @Override
        public boolean nextKeyValue() throws IOException, InterruptedException {
            String continuing_line = new String();
            for (int i = 0; i < tweet_length; i++) {
                if (!line_rr.nextKeyValue()) {
                    return false;
                }
                continuing_line = continuing_line.concat(line_rr.getCurrentValue().toString()).concat("\n");
            }
            
            currentValue.set(continuing_line);
            return true;
        }


        @Override
        public LongWritable getCurrentKey() throws IOException, InterruptedException {
            return currentKey;
        }


        @Override
        public Text getCurrentValue() throws IOException, InterruptedException {
            return currentValue;
        }


        @Override
        public float getProgress() throws IOException, InterruptedException {
            return line_rr.getProgress();
        }


        @Override
        public void close() throws IOException {
            line_rr.close();
        }

    }

    public static class TweetInputFormat extends FileInputFormat<LongWritable, Text> {

        @Override
        public RecordReader<LongWritable, Text> createRecordReader(InputSplit split, TaskAttemptContext context) {
            return new TweetRecordReader();
        }
}


    public static void main(String[] args) throws Exception {

        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Read tweets line by line");
        job.setJarByClass(WordCount.class);
        //job.setMapperClass(ReadLineMapper.class); // uncomment to for other map-reduce
        job.setInputFormatClass(TweetInputFormat.class); //comment for other map-reduce
        job.setMapperClass(ReadTweetMapper.class); //commnet for other map-reduce
        job.setReducerClass(PassThroughReducer.class);
        

        job.setOutputKeyClass(IntWritable.class);
        job.setOutputValueClass(IntWritable.class);

        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));

        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}
