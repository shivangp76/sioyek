
package info.sioyek.sioyek;

import org.qtproject.qt.android.QtNative;

import org.qtproject.qt.android.bindings.QtActivity;
import android.os.*;
import android.provider.Settings;
import android.os.Environment;
import android.database.Cursor;
import android.provider.MediaStore;
import android.provider.DocumentsContract;
import android.content.*;
import android.app.*;
import android.view.WindowManager;
import android.widget.Toast;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.app.ActivityManager;
import android.view.ViewTreeObserver;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.String;
import android.content.Intent;
import java.io.File;

import java.lang.String;
import android.content.Intent;
import java.io.File;
import android.net.Uri;
import android.util.Log;
import android.content.ContentResolver;
import android.webkit.MimeTypeMap;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.graphics.Rect;


import androidx.appcompat.app.AppCompatActivity;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.media3.session.MediaController;
import androidx.media3.session.SessionToken;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.MoreExecutors;

import android.view.Menu;
import android.view.MenuItem;



public class SioyekActivity extends QtActivity{
    public static native void setFileUrlReceived(String url);
    public static native void qDebug(String msg);
    public static native void onTts(int begin, int end);
    public static native void onTtsStateChange(String newState);
    public static native void onExternalTtsStateChange(String newState);
    public static native String getRestOnPause();
    public static native boolean onResumeState(boolean isPlaying, boolean readingRest, int offset);
    public static native void onAndroidPause();
    public static native void onAndroidResume();
    public static native void onAndroidKeypadHide();
    public static native void onAndroidKeypadShow(int size);

    public static boolean isIntentPending;
    public static boolean isInitialized;
    // public static boolean wasPlayingWhenPaused = false;
    public static boolean isPaused = true;

    private static SioyekActivity instance = null;

    private MediaController mediaController = null;
    private SessionToken ttsSessionToken = null;
    private int prevKeypadHeight = 0;

    public void setKeyboardListener(final Activity activity) {
        final View rootView = activity.findViewById(android.R.id.content);
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                qDebug("sioyek: something happened");
                Rect r = new Rect();
                rootView.getWindowVisibleDisplayFrame(r);
                int screenHeight = rootView.getRootView().getHeight();
                int keypadHeight = screenHeight - r.bottom;
                if (keypadHeight == 0 && prevKeypadHeight != 0){
                    onAndroidKeypadHide();

                }
                if (keypadHeight != 0 && prevKeypadHeight == 0){
                    onAndroidKeypadShow(keypadHeight);
                }

                prevKeypadHeight = keypadHeight;
                // qDebug("sioyek: keypadHeight: " + keypadHeight);

                // Rect r = new Rect();
                // rootView.getWindowVisibleDisplayFrame(r);
                // int screenHeight = rootView.getRootView().getHeight();
                // int keypadHeight = screenHeight - r.bottom;

                // nativeKeyboardHeightChanged(nativePtr, keypadHeight);
            }
        });
    }

    // private static native void nativeKeyboardHeightChanged(long nativePtr, int height);

    private BroadcastReceiver messageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int begin = intent.getIntExtra("begin", 0);
            int end = intent.getIntExtra("end", 0);
            onTts(begin, end);
        }
    };

    private BroadcastReceiver resumeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            boolean isPlaying = intent.getBooleanExtra("isPlaying", false);
            boolean isOnRest = intent.getBooleanExtra("isOnRest", false);
            int offset = intent.getIntExtra("offset", 0);
            // run onResumeState after a delay to make sure that the activity is fully resumed
            boolean result = onResumeState(isPlaying, isOnRest, offset);
            if (!result){
                // the app is not fully initialized yet, try again after a delay
                Handler handler = new Handler(Looper.getMainLooper());
                handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        onResumeState(isPlaying, isOnRest, offset);
                    }
                }, 1000);
            }
        }
    };

    private BroadcastReceiver stateMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String state = intent.getStringExtra("state");
            onTtsStateChange(state);
        }
    };

    private BroadcastReceiver externalStateMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String state = intent.getStringExtra("state");
            onExternalTtsStateChange(state);
            //onTtsStateChange(state);
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);


        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Intent intent = getIntent();

        if (intent != null){
            String action = intent.getAction();
            if (action != null){
                Uri intentUri = intent.getData();
                if (intentUri != null){
                    if (intentUri.toString().startsWith("content://") && (!intentUri.toString().startsWith("content://com.android")) && (!intentUri.toString().startsWith("content://media")) && (intentUri.toString().indexOf("@media") == -1)){
                        //Toast.makeText(this, "Opening files from other apps is not supported. Download the file and open it from file manager.", Toast.LENGTH_LONG).show();

                        Intent viewIntent = new Intent(getApplicationContext(), SioyekActivity.class);
                        // viewIntent.setUri(intentUri);
                        viewIntent.setAction(Intent.ACTION_VIEW);
                        viewIntent.putExtra("sharedData", intentUri.toString());

                        startActivity(viewIntent);
                        if (instance != null){
                            finish();
                        }
                    }
                    isIntentPending = true;
                }
            }
        }

        instance = this;
        if(!Environment.isExternalStorageManager()){

            String packageName = getApplicationContext().getPackageName();
            // Uri uri = Uri.parse("package:" + BuildConfig.APPLICATION_ID);
            // Uri uri = Uri.parse("package:" + "org.qtproject.example");
            Uri uri = Uri.parse("package:" + packageName);
            try {
                Intent newActivityIntent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri);

                startActivity(
                    newActivityIntent
                );
            }
            catch(Exception e){
            }
        }

        setKeyboardListener(this);

    }

    ListenableFuture<MediaController> ensureTtsService(){
        // start the TTS service if it doesn't already exist
        if (mediaController == null){
            return startTtsService();
        }
        return Futures.immediateFuture(mediaController);
    }

    ListenableFuture<MediaController> startTtsService(){
        ttsSessionToken = new SessionToken(getApplicationContext(), new ComponentName(getApplicationContext(), TextToSpeechService.class));
        ListenableFuture<MediaController> controllerFuture = new MediaController.Builder(getApplicationContext(), ttsSessionToken).buildAsync();
        controllerFuture.addListener(() -> {
            try{
                mediaController = controllerFuture.get();
            }
            catch(Exception e){
                qDebug("sioyek: could not get media controller");
            }
        }, MoreExecutors.directExecutor());
        return controllerFuture;
    }

    @Override
    public void onStart(){
        super.onStart();
        registerReceiver(messageReceiver, new IntentFilter("info.sioyek.sioyek.SIOYEK_TTS"), Context.RECEIVER_EXPORTED);
        registerReceiver(stateMessageReceiver, new IntentFilter("info.sioyek.sioyek.SIOYEK_TTS_STATE"), Context.RECEIVER_EXPORTED);
        registerReceiver(externalStateMessageReceiver, new IntentFilter("info.sioyek.sioyek.SIOYEK_EXTERNAL_TTS_STATE"), Context.RECEIVER_EXPORTED);
    }

    @Override
    public void onStop(){
        super.onStop();
        unregisterReceiver(messageReceiver);
        unregisterReceiver(stateMessageReceiver);
        unregisterReceiver(externalStateMessageReceiver);
    }

    public void startMaybeForegroundService(Intent intent){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    boolean isTtsServiceRunning(){

        ActivityManager activityManager = (ActivityManager) getApplicationContext().getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : activityManager.getRunningServices(Integer.MAX_VALUE)) {
            if (service.service.getClassName().equals(TextToSpeechService.class.getName())) {
                return true;
            }
        }
        return false;
    }

    @Override
    public void onResume(){

        IntentFilter filter = new IntentFilter("info.sioyek.sioyek.SIOYEK_RESUME");
        registerReceiver(resumeReceiver, filter, Context.RECEIVER_EXPORTED);
        boolean wasTtsServiceRunning = isTtsServiceRunning();
        // Log.d("sioyek", "wasTtsServiceRunning: " + wasTtsServiceRunning);
        
        isPaused = false;
        // if (wasPlayingWhenPaused){
        if (wasTtsServiceRunning){
            // wasPlayingWhenPaused = false;
            Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
            intent.putExtra("resume", true);
            startMaybeForegroundService(intent);

        }

        super.onResume();
        onAndroidResume();



    }

    @Override
    public void onPause(){
        isPaused = true;
        String rest = getRestOnPause();
        // the format of rest is "<offset> <rest>" where offset is an integer offset of begin position

        if (rest.length() > 0){
            String[] parts = rest.split(" ", 2);
            int offset = Integer.parseInt(parts[0]);
            String restString = parts[1];
            ensureTtsService().addListener(() -> {
                // wasPlayingWhenPaused = true;
                setTtsRestOfDocument(restString, offset);
            }, MoreExecutors.directExecutor());
        }

        onAndroidPause();
        super.onPause();

        unregisterReceiver(resumeReceiver);

    }

    @Override
    public void onNewIntent(Intent intent){
        super.onNewIntent(intent);
        setIntent(intent);
        if (isInitialized){
            processIntent();
        }
        else{
            isIntentPending = true;
        }
    }

    public void checkPendingIntents(String workingDir){
        isInitialized = true;
        if (isIntentPending){
            isIntentPending = false;
            processIntent();
        }
    }

    public static File getFile(Context context, Uri uri) throws IOException {
        File destinationFilename = new File(context.getFilesDir().getPath() + File.separatorChar + queryName(context, uri));

        if (destinationFilename.exists()){
            return destinationFilename;
        }

        try (InputStream ins = context.getContentResolver().openInputStream(uri)) {
            createFileFromStream(ins, destinationFilename);
        } catch (Exception ex) {
            Log.e("Save File", ex.getMessage());
            ex.printStackTrace();
        }
        return destinationFilename;
    }

    public static void createFileFromStream(InputStream ins, File destination) {
        try (OutputStream os = new FileOutputStream(destination)) {
            byte[] buffer = new byte[4096];
            int length;
            while ((length = ins.read(buffer)) > 0) {
                os.write(buffer, 0, length);
            }
            os.flush();
        } catch (Exception ex) {
            Log.e("Save File", ex.getMessage());
            ex.printStackTrace();
        }
    }

    private static String queryName(Context context, Uri uri) {
        Cursor returnCursor =
                context.getContentResolver().query(uri, null, null, null, null);
        assert returnCursor != null;
        int nameIndex = returnCursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
        returnCursor.moveToFirst();
        String name = returnCursor.getString(nameIndex);
        returnCursor.close();
        return name;
    }

    public static String getRealPathFromUri(Context context, Uri contentUri) throws IOException{
        Cursor cursor = null;
        try {
            String[] proj = { MediaStore.Images.Media.DATA };
            cursor = context.getContentResolver().query(contentUri, proj, null, null, null);
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        }
        catch(Exception e){
            String newFileName=  getFile(context, contentUri).getPath();
            return getFile(context, contentUri).getPath();
        }
        finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    public static String getPathFromUri(Context context, Uri uri) {
        final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        // DocumentProvider
        if (isKitKat && DocumentsContract.isDocumentUri(context, uri)) {
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                if ("primary".equalsIgnoreCase(type)) {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                }

                // TODO handle non-primary volumes
            }
            // DownloadsProvider
            else if (isDownloadsDocument(uri)) {

                final String id = DocumentsContract.getDocumentId(uri);
                final Uri contentUri = ContentUris.withAppendedId(
                        Uri.parse("content://downloads/public_downloads"), Long.valueOf(id));

                return getDataColumn(context, contentUri, null, null);
            }
            // MediaProvider
            else if (isMediaDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                Uri contentUri = null;
                if ("image".equals(type)) {
                    contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                } else if ("video".equals(type)) {
                    contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                } else if ("audio".equals(type)) {
                    contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                }

                final String selection = "_id=?";
                final String[] selectionArgs = new String[] {
                        split[1]
                };

                return getDataColumn(context, contentUri, selection, selectionArgs);
            }
        }
        // MediaStore (and general)
        else if ("content".equalsIgnoreCase(uri.getScheme())) {

            // Return the remote address
            if (isGooglePhotosUri(uri))
                return uri.getLastPathSegment();

            return getDataColumn(context, uri, null, null);
        }
        // File
        else if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        }

        return null;
    }

    public static String getDataColumn(Context context, Uri uri, String selection,
                                       String[] selectionArgs) {

        Cursor cursor = null;
        final String column = "_data";
        final String[] projection = {
                column
        };

        try {
            cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                final int index = cursor.getColumnIndexOrThrow(column);
                return cursor.getString(index);
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }
        return null;
    }


    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is ExternalStorageProvider.
     */
    public static boolean isExternalStorageDocument(Uri uri) {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is DownloadsProvider.
     */
    public static boolean isDownloadsDocument(Uri uri) {
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is MediaProvider.
     */
    public static boolean isMediaDocument(Uri uri) {
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is Google Photos.
     */
    public static boolean isGooglePhotosUri(Uri uri) {
        return "com.google.android.apps.photos.content".equals(uri.getAuthority());
    }

    private void processIntent(){

        Intent intent = getIntent();
        if (intent.getAction().equals("android.intent.action.VIEW")){
            Uri intentUri = intent.getData();
            if (intentUri == null){
                intentUri = Uri.parse(intent.getStringExtra("sharedData"));
            }
            //String realPath = getRealPathFromUri(getApplicationContext(), intentUri);
            //Uri newUri = Uri.fromFile(new File(realPath));

            //setFileUrlReceived(intentUri.toString());
            String realPath = "";
            try{
                realPath = getRealPathFromUri(this, intentUri);
                //Toast.makeText(this, "trying to open " + realPath, Toast.LENGTH_LONG).show();
                setFileUrlReceived(realPath);
            }
            catch(IOException e){
                qDebug("sioyek: could not open" + realPath);
            }
        }
        return;
    }

    public void ttsPause(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mediaController != null){
                    mediaController.pause();
                }
            }
        });

    } 

    public int ttsGetMaxTextSize(){
        return TextToSpeech.getMaxSpeechInputLength();
    }

    public boolean ttsIsPlaying(){
        return mediaController != null && mediaController.getPlayerState() == 2;
    }

    public void ttsStop(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mediaController != null){
                    mediaController.stop();
                }
            }
        });
    } 

    public void ttsSetRate(float rate){
        ensureTtsService().addListener(() -> {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    setTtsRate(rate);
                }
            });
        }, MoreExecutors.directExecutor());
    } 

    public void ttsSay(String text, int startOffset){
        ensureTtsService().addListener(() -> {
            Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
            intent.putExtra("text", text);
            intent.putExtra("startOffset", startOffset);
            startMaybeForegroundService(intent);

            Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (mediaController != null){
                        mediaController.play();
                    }
                }
            }, 100);
        }, MoreExecutors.directExecutor());
    }

    public void myLogD(String msg){
        Log.d("sioyek", msg);
    }

    public void setTtsRate(float rate){
        ensureTtsService().addListener(() -> {
            Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
            intent.putExtra("rate", rate);
            startMaybeForegroundService(intent);
        }, MoreExecutors.directExecutor());
    }

    public void setTtsRestOfDocument(String rest, int offset){
        ensureTtsService().addListener(() -> {
            Intent intent = new Intent(getApplicationContext(), TextToSpeechService.class);
            intent.putExtra("rest", rest);
            intent.putExtra("restOffset", offset);
            startMaybeForegroundService(intent);
        }, MoreExecutors.directExecutor());
    }

    public void setScreenBrightness(float brightness){
        runOnUiThread(
            new Runnable() {
                @Override
                public void run() {
                    WindowManager.LayoutParams lp = getWindow().getAttributes();
                    lp.screenBrightness = brightness;
                    getWindow().setAttributes(lp);
                }
            }
        );
    }

    float getScreenBrightness(){
        WindowManager.LayoutParams lp = getWindow().getAttributes();
        if (lp.screenBrightness == -1){
            try{
                float currentBrightness = Settings.System.getFloat(getContentResolver(), Settings.System.SCREEN_BRIGHTNESS);
                return currentBrightness / 255.0f;
            }
            catch(Settings.SettingNotFoundException e){
                return 0.5f;
            }
        }
        else{
            return lp.screenBrightness;
        }
    }

}
