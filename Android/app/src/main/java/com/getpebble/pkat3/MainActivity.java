package com.getpebble.pkat3;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;


public class MainActivity extends Activity {

    private static final UUID APP_UUID = UUID.fromString("af17efe7-2141-4eb2-b62a-19fc1b595595");

    private PebbleKit.PebbleDataReceiver mDataReceiver;

    private ImageView mChoiceView;
    private TextView mInstructionView;
    private Button mRockButton;
    private Button mPaperButton;
    private Button mScissorsButton;
    private Handler mHandler = new Handler();

    private int mChoice = Keys.CHOICE_WAITING;
    private int mP2Choice = Keys.CHOICE_WAITING;
    private int mGameCounter, mWinCounter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Get UI elements for manipulation
        mChoiceView = (ImageView)findViewById(R.id.choice_view);
        mInstructionView = (TextView)findViewById(R.id.instructions);
        mRockButton = (Button)findViewById(R.id.rock_button);
        mPaperButton = (Button)findViewById(R.id.paper_button);
        mScissorsButton = (Button)findViewById(R.id.scissors_button);

        mRockButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                mChoice = Keys.CHOICE_ROCK;
                updateUI();
            }

        });

        mPaperButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                mChoice = Keys.CHOICE_PAPER;
                updateUI();
            }

        });

        mScissorsButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                mChoice = Keys.CHOICE_SCISSORS;
                updateUI();
            }

        });

        updateUI();
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Reset game
        mChoice = Keys.CHOICE_WAITING;
        mWinCounter = 0;
        mGameCounter = 0;
        updateUI();

        // Register to get updates from Pebble
        if(mDataReceiver == null) {
            mDataReceiver = new PebbleKit.PebbleDataReceiver(APP_UUID) {

                @Override
                public void receiveData(Context context, int transactionId, PebbleDictionary dict) {
                    // Always ACK
                    PebbleKit.sendAckToPebble(context, transactionId);

                    // Was a choice received from player 2?
                    if(dict.getInteger(Keys.KEY_CHOICE) != null) {
                        mP2Choice = dict.getInteger(Keys.KEY_CHOICE).intValue();

                        if(mChoice != Keys.CHOICE_WAITING) {
                            // A match can be played!
                            doMatch();
                        }
                    }
                }

            };
            PebbleKit.registerReceivedDataHandler(getApplicationContext(), mDataReceiver);
        }
    }

    /**
     * Compare choices and declare a winner
     * R = R
     * R < P
     * R > S
     *
     * P > R
     * P = P
     * P < S
     *
     * S < R
     * S > P
     * S = S
     */
    private void doMatch() {
        // Remember how many games in this session
        mGameCounter++;

        PebbleDictionary resultDict = new PebbleDictionary();

        switch (mChoice) {
            case Keys.CHOICE_ROCK:
                switch(mP2Choice) {
                    case Keys.CHOICE_ROCK:
                        mInstructionView.setText("It's a tie!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_TIE);
                        break;
                    case Keys.CHOICE_PAPER:
                        mInstructionView.setText("You lose!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_WIN);  // Inform Pebble of opposite result
                        break;
                    case Keys.CHOICE_SCISSORS:
                        mWinCounter++;
                        mInstructionView.setText("You win! (" + mWinCounter + " of " + mGameCounter + ")");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_LOSE);
                        break;
                }
                break;
            case Keys.CHOICE_PAPER:
                switch(mP2Choice) {
                    case Keys.CHOICE_ROCK:
                        mWinCounter++;
                        mInstructionView.setText("You win! (" + mWinCounter + " of " + mGameCounter + ")");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_LOSE);
                        break;
                    case Keys.CHOICE_PAPER:
                        mInstructionView.setText("It's a tie!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_TIE);
                        break;
                    case Keys.CHOICE_SCISSORS:
                        mInstructionView.setText("You lose!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_WIN);
                        break;
                }
                break;
            case Keys.CHOICE_SCISSORS:
                switch(mP2Choice) {
                    case Keys.CHOICE_ROCK:
                        mInstructionView.setText("You lose!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_WIN);
                        break;
                    case Keys.CHOICE_PAPER:
                        mWinCounter++;
                        mInstructionView.setText("You win! (" + mWinCounter + " of " + mGameCounter + ")");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_LOSE);
                        break;
                    case Keys.CHOICE_SCISSORS:
                        mInstructionView.setText("It's a tie!");
                        resultDict.addInt32(Keys.KEY_RESULT, Keys.RESULT_TIE);
                        break;
                }
                break;
        }

        // Inform Pebble of result
        PebbleKit.sendDataToPebble(getApplicationContext(), APP_UUID, resultDict);

        // Finally reset both
        mChoice = Keys.CHOICE_WAITING;
        mP2Choice = Keys.CHOICE_WAITING;

        // Leave announcement for 5 seconds
        mHandler.postDelayed(new Runnable() {

            @Override
            public void run() {
                updateUI();
            }

        }, 5000);
    }

    /**
     * Update UI elements based on the current state of the game
     */
    private void updateUI() {
        if(mChoice == Keys.CHOICE_WAITING) {
            mInstructionView.setText("Choose your weapon...");
            mChoiceView.setImageResource(R.drawable.unknown);
            mRockButton.setEnabled(true);
            mPaperButton.setEnabled(true);
            mScissorsButton.setEnabled(true);
        } else {
            // A mChoice has been made
            mRockButton.setEnabled(false);
            mPaperButton.setEnabled(false);
            mScissorsButton.setEnabled(false);

            mInstructionView.setText("Waiting for opponent...");

            switch(mChoice) {
                case Keys.CHOICE_ROCK:
                    mChoiceView.setImageResource(R.drawable.rock);
                    break;
                case Keys.CHOICE_PAPER:
                    mChoiceView.setImageResource(R.drawable.paper);
                    break;
                case Keys.CHOICE_SCISSORS:
                    mChoiceView.setImageResource(R.drawable.scissors);
                    break;
            }
        }

        // Check Pebble player response has arrived first
        if(mChoice != Keys.CHOICE_WAITING && mP2Choice != Keys.CHOICE_WAITING) {
            doMatch();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.action_install:
                sideloadInstall(getApplicationContext(), "PKAT3-pebble.pbw");
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Alternative sideloading method
     * Source: http://forums.getpebble.com/discussion/comment/103733/#Comment_103733
     */
    public static void sideloadInstall(Context ctx, String assetFilename) {
        try {
            // Read .pbw from assets
            Intent intent = new Intent(Intent.ACTION_VIEW);
            File file = new File(ctx.getExternalFilesDir(null), assetFilename);
            InputStream is = ctx.getResources().getAssets().open(assetFilename);
            OutputStream os = new FileOutputStream(file);
            byte[] pbw = new byte[is.available()];
            is.read(pbw);
            os.write(pbw);
            is.close();
            os.close();

            // Install via Pebble Android app
            intent.setDataAndType(Uri.fromFile(file), "application/pbw");
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(intent);
        } catch (Exception e) {
            Toast.makeText(ctx, "App install failed: " + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }
    }
}
