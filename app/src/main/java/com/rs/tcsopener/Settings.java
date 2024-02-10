package com.rs.tcsopener;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class Settings extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        Button set = findViewById(R.id.button);
        Button logRef = findViewById(R.id.button2);
        Button logClr = findViewById(R.id.button3);
        EditText broker = findViewById(R.id.broker);
        TextView debugLog = findViewById(R.id.textView3);

        SharedPreferences settings = getApplicationContext().getSharedPreferences("app_settings", 0);
        String brokerString = settings.getString("broker", BuildConfig.MQTT_DEFAULT_BROKER_IP).trim().replaceAll(" ","");
        broker.setText(brokerString);
        String log = settings.getString("log", "");
        log = log.replaceAll(",","\n");
        debugLog.setText(log);

        logClr.setOnClickListener(v -> {
            SharedPreferences.Editor editor = settings.edit();
            editor.remove("log");
            editor.apply();
            debugLog.setText("");
        });

        logRef.setOnClickListener(v -> {
            String log1 = settings.getString("log", "");
            log1 = log1.replaceAll(",","\n");
            debugLog.setText(log1);
        });

        set.setOnClickListener(v -> {
            if(broker.getText() == null || broker.getText().toString().equals("")) return;
            SharedPreferences.Editor editor = settings.edit();
            editor.putString("broker", broker.getText().toString());
            editor.apply();
        });
    }
}