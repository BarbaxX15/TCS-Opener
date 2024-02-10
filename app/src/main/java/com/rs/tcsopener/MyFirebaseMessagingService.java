package com.rs.tcsopener;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

public class MyFirebaseMessagingService extends FirebaseMessagingService {
    private static final String TCS_SERIAL_NUMBER =  BuildConfig.TCS_SERIAL_NUMBER;
    private static final String NOTIFICATION_TITLE_FLOOR = "Ding Dong (Wohnungstür)";
    private static final String NOTIFICATION_TITLE_MAIN = "Ding Dong (Haustür)";



    private Context mContext;
    private Notification.Builder mBuilder;
    private NotificationManager mNotificationManager;
    private static final String bellIconString = new String(Character.toChars(0x1F514));

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getApplicationContext();
        mNotificationManager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        buildNotification();
    }

    @Override
    public void onNewToken(@NonNull String token) {
        super.onNewToken(token);
    }

    @Override
    public void onMessageReceived(@NonNull RemoteMessage message) {
        super.onMessageReceived(message);
        Object obj = message.getData().get("id");
        if (obj != null) {
            ringAction(obj.toString());
        }
    }

    private void buildNotification(){
        mBuilder = new Notification.Builder(mContext.getApplicationContext(), "TCS");
        Intent notificationIntent = new Intent(mContext.getApplicationContext(),MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0, notificationIntent,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
        NotificationChannel channel = new NotificationChannel(
                "TCS Opener",
                "Doorbell",
                NotificationManager.IMPORTANCE_HIGH);
        mNotificationManager.createNotificationChannel(channel);

        mBuilder
                .setContentIntent(pendingIntent)
                .setAutoCancel(true)
                .setSmallIcon(R.drawable.tcs_tuercontrolsysteme_ag_logo_vector)
                .setShowWhen(true)
                .setContentTitle("TCS Opener")
                .setContentText("DEBUG")
                .setChannelId("TCS Opener");
    }

    private void ringAction(String message){
        if(message.indexOf(TCS_SERIAL_NUMBER)!= 1) return;
        if(message.charAt(0) == '1'){
            // Floor bell button
            mBuilder.setContentText(bellIconString + "   " + NOTIFICATION_TITLE_FLOOR + "   " + bellIconString);
        } else {
            // House bell button
            mBuilder.setContentText(bellIconString + "   " + NOTIFICATION_TITLE_MAIN + "   " + bellIconString);
        }
        mNotificationManager.notify(0, mBuilder.build());
        LocalBroadcastManager.getInstance(this).sendBroadcast(new Intent("RINGING"));
    }
}
