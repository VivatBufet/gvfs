#include <string.h>
#include <unistd.h>

#include <glib.h>
#include "gfile.h"
#include "glocalfileinputstream.h"
#include "glocalfileoutputstream.h"
#include "gsocketinputstream.h"

static void
test_out ()
{
  GOutputStream *out;
  char buffer[2345];
  char *ptr;
  char *str = "Test_String ";
  int str_len;
  int left;
  int i;
  gssize res;
  gboolean close_res;
  GError *error;

  str_len = strlen (str);
  for (i = 0; i < sizeof(buffer); i++) {
    buffer[i] = str[i%str_len];
  }

  g_print ("test_out\n");
  
  unlink ("/tmp/test");
  
  out = (GOutputStream *)g_local_file_output_stream_new ("/tmp/test",
							 G_OUTPUT_STREAM_OPEN_MODE_CREATE);

  left = sizeof(buffer);
  ptr = buffer;
  
  while (left > 0)
    {
      error = NULL;
      res = g_output_stream_write (out, ptr, MIN (left, 128), &error);
      g_print ("res = %d\n", res);

      if (res == -1)
	{
	  g_print ("error %d: %s\n", error->code, error->message);
	  g_error_free (error);
	}
      
      if (res > 0)
	{
	  left -= res;
	  ptr += res;
	}

      if (res < 0)
	break;
    }

  close_res = g_output_stream_close (out, NULL);
  g_print ("close res: %d\n", close_res);
}

static void
test_sync (char *filename, gboolean dump)
{
  GInputStream *in;
  char buffer[1025];
  gssize res;
  gboolean close_res;
  
  in = (GInputStream *)g_local_file_input_stream_new (filename);

  while (1)
    {
      res = g_input_stream_read (in, buffer, 1024, NULL);
      if (dump)
	{
	  if (res > 0)
	    {
	      buffer[res] = 0;
	      g_print ("%s", buffer);
	    }
	}
      else
	g_print ("res = %d\n", res);

      if (res <= 0)
	break;
    }

  close_res = g_input_stream_close (in, NULL);

  if (!dump)
    g_print ("close res: %d\n", close_res);
}

static void
close_done (GInputStream *stream,
	    gboolean      result,
	    gpointer      data,
	    GError       *error)
{
  g_print ("close result: %d\n", result);
}

static void
read_done (GInputStream *stream,
	   void         *buffer,
	   gsize         count_requested,
	   gssize        count_read,
	   gpointer      data,
	   GError       *error)
{
  g_print ("count_read: %d\n", count_read);
  if (count_read == -1)
    g_print ("Error %d: %s\n", error->code, error->message);

  if (count_read > 0)
    {
      g_input_stream_read_async (stream, buffer, 1024, 0, read_done, buffer, NULL);
      //g_input_stream_cancel (stream);
    }
  else
    g_input_stream_close_async (stream, 0, close_done, buffer, g_free);
}


static void
test_async (char *filename, gboolean dump)
{
  GInputStream *in;
  char *buffer;

  buffer = g_malloc (1025);
  
  in = (GInputStream *)g_local_file_input_stream_new (filename);
  
  g_input_stream_read_async (in, buffer, 1024, 0, read_done, buffer, NULL);
}

static gboolean
cancel_stream (gpointer data)
{
  g_input_stream_cancel (G_INPUT_STREAM (data));
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GFile *file;
  GMainLoop *loop;

  g_type_init ();
  g_thread_init (NULL);

  loop = g_main_loop_new (NULL, FALSE);

  if (0) {
    GInputStream *s;
    char *buffer;
    //gssize res;

    buffer = g_malloc (1025);
    
    s = g_socket_input_stream_new (0, FALSE);

    /*
    res = g_input_stream_read (s, buffer, 128, NULL);
    g_print ("res1: %d\n", res);
    res = g_input_stream_read (s, buffer, 128, NULL);
    g_print ("res2: %d\n", res);
    */
    
    g_input_stream_read_async (s, buffer, 128, 0, read_done, buffer, NULL);
    g_timeout_add (1000, cancel_stream, s);
    g_print ("main loop run\n");
    g_main_loop_run (loop);
    g_print ("main loop quit\n");
  }

  
  file = g_file_get_for_path ("/tmp");

  if (1) test_sync ("/etc/passwd", FALSE);
  if (1) test_async ("/etc/passwd", TRUE);

  test_out ();

  g_main_loop_run (loop);
  
  return 0;
}
