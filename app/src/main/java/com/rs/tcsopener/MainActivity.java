package com.rs.tcsopener;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.google.firebase.messaging.FirebaseMessaging;
import com.ncorti.slidetoact.SlideToActView;

import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

import info.mqtt.android.service.Ack;
import info.mqtt.android.service.MqttAndroidClient;
import pl.droidsonroids.gif.GifDrawable;


public class MainActivity extends AppCompatActivity {
    private static final String FIREBASE_TOPIC = "doorbell";
    private static final String MQTT_DEFAULT_BROKER_IP = BuildConfig.MQTT_DEFAULT_BROKER_IP;
    private static final String MQTT_OPEN_MESSAGE_TO_ESP = "open";
    private static final String MQTT_OPEN_TOPIC = "home/open";
    private static final String MQTT_DEBUG_TOPIC = "home/debug";
    private static final String MQTT_USERNAME =  BuildConfig.MQTT_USERNAME;
    private static final String MQTT_PASSWORD =  BuildConfig.MQTT_PASSWORD;



    private Context mContext;
    public MqttAndroidClient mClient;
    private SharedPreferences settings;
    private TextView status;
    private boolean isConnected = false;

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mClient != null && mClient.isConnected()) mClient.disconnect();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(mClient != null && mClient.isConnected()) mClient.disconnect();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(mClient == null || !mClient.isConnected()) connectMQTT();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mContext = getApplicationContext();

        FirebaseMessaging.getInstance().subscribeToTopic(FIREBASE_TOPIC);

        ImageButton settingsButton = findViewById(R.id.settings);
        settingsButton.setOnClickListener(v -> startActivity(new Intent(mContext,Settings.class)));

        status = findViewById(R.id.status);
        ImageView imageView = findViewById(R.id.imageView3);
        SlideToActView mSlide = findViewById(R.id.example);

        GifDrawable mGif;
        try {
            mGif = new GifDrawable( getResources(), R.drawable.image_processing20210911_11529_1ka8b1i );
        } catch (IOException e) {
            return;
        }
        imageView.setImageDrawable(mGif);
        mGif.stop();

        mSlide.setOnSlideCompleteListener(slideToActView -> {
            // Action on Slide
            if(isConnected){
                mClient.publish(MQTT_OPEN_TOPIC,new MqttMessage(MQTT_OPEN_MESSAGE_TO_ESP.getBytes(StandardCharsets.UTF_8)));
                new Handler(Looper.getMainLooper()).postDelayed(mSlide::resetSlider, 2000);
            } else {
                mSlide.resetSlider();
            }
        });


        // INTENT FROM SERVICE IF APP IS OPEN WHILE RECEIVING NOTIFICATION
        IntentFilter filter = new IntentFilter("RINGING");
        LocalBroadcastManager.getInstance(this).registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        if (intent.getAction() == null) return;
                        if (intent.getAction().equals("RINGING")) {
                            mGif.start();
                            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    mGif.stop();
                                }
                            }, 5000);
                        }
                    }
                },
                filter
        );

        settings = getApplicationContext().getSharedPreferences("app_settings", 0);
        connectMQTT();
    }

    private void connectMQTT(){
        mClient = new MqttAndroidClient(mContext,"tcp://"+ settings.getString("broker", MQTT_DEFAULT_BROKER_IP), MqttClient.generateClientId(), Ack.AUTO_ACK);
        mClient.setCallback(mClientCallback);
        try {
            MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
            mqttConnectOptions.setUserName(MQTT_USERNAME);
            mqttConnectOptions.setPassword(MQTT_PASSWORD.toCharArray());
            mqttConnectOptions.setCleanSession(true);
            mqttConnectOptions.setConnectionTimeout(5);
            mClient.connect(mqttConnectOptions, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    String connectMessage = "Connected with ID "+mClient.getClientId();
                    mClient.publish(MQTT_DEBUG_TOPIC, new MqttMessage(connectMessage.getBytes(StandardCharsets.UTF_8)));
                    subscribeTopic(MQTT_DEBUG_TOPIC);
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    status.setBackgroundColor(Color.parseColor("#FF0000"));
                    Toast.makeText(mContext, "Could not connect to MQTT", Toast.LENGTH_SHORT).show();
                    isConnected = false;
                }
            });

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private final MqttCallback mClientCallback = new MqttCallback() {
        @Override
        public void connectionLost(Throwable cause) {
            status.setBackgroundColor(Color.parseColor("#FF0000"));
            isConnected = false;
            new Handler(Looper.getMainLooper()).postDelayed(() -> {
                connectMQTT();
            }, 5000);
        }

        @Override
        public void messageArrived(String topic, MqttMessage message) throws Exception {
            String sMessage = message.toString();
            String[] log = settings.getString("log", "").split(",");
            ArrayList<String> logList = new ArrayList<String>(Arrays.asList(log));
            if (logList.size() > 100) logList.clear();
            SimpleDateFormat sdf = new SimpleDateFormat("dd.MM.yyyy HH:mm:ss", Locale.getDefault());
            String currentDateandTime = sdf.format(new Date());
            logList.add(currentDateandTime + " | " + topic + ": " + sMessage);
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < logList.size(); i++) {
                sb.append(logList.get(i)).append(",");
            }
            SharedPreferences.Editor editor = settings.edit();
            editor.putString("log", sb.toString());
            editor.apply();
        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken token) {

        }
    };

    public void subscribeTopic(String topic) {
        try {
            mClient.subscribe(topic, 0, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    status.setBackgroundColor(Color.parseColor("#7CFC00"));
                    isConnected = true;
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    status.setBackgroundColor(Color.parseColor("#FF0000"));
                    isConnected = false;
                }
            });

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}