#!/usr/bin/env python
# coding: utf-8

# In[ ]:


from __future__ import unicode_literals
import os
from flask import Flask, request, abort
from linebot import LineBotApi, WebhookHandler,WebhookParser
from linebot.exceptions import InvalidSignatureError
from linebot.models import (
    MessageEvent, 
    TextMessage, 
    TextSendMessage,
    ImageMessage,
    ImageSendMessage,
    TemplateSendMessage,
    ButtonsTemplate,
    MessageTemplateAction,
    PostbackEvent,
    PostbackTemplateAction,
    LocationSendMessage
)

import configparser
import requests
import random

flag = 0
score = 0
time = 0
password = ["5","2","0","9"]
app = Flask(__name__)

# LINE 聊天機器人的基本資料
config = configparser.ConfigParser()
config.read('config.ini')

line_bot_api = LineBotApi(config.get('line-bot', 'channel_access_token'))
handler = WebhookHandler(config.get('line-bot', 'channel_secret'))
parser = WebhookParser(config.get('line-bot', 'channel_secret'))


# 接收 LINE 的資訊
@app.route("/callback", methods=['POST'])
def callback():
    global flag
    global score
    global time
    signature = request.headers['X-Line-Signature']

    body = request.get_data(as_text=True)
    app.logger.info("Request body: " + body)
    
    try:
        events = parser.parse(body, signature)  # 傳入的事件
        
    except InvalidSignatureError:
        abort(400)
    for event in events:
        url= 'https://api.thingspeak.com/channels/1620834/feeds.json?api_key=NCFYKMBO44O15L9Y&results=2'
        laturl = 'https://api.thingspeak.com/channels/1620834/fields/1.json?api_key=NCFYKMBO44O15L9Y&results=2'
        logurl = 'https://api.thingspeak.com/channels/1620834/fields/2.json?api_key=NCFYKMBO44O15L9Y&results=2'
        lckurl = 'https://api.thingspeak.com/channels/1620834/fields/3.json?api_key=NCFYKMBO44O15L9Y&results=2'
        uurl = "http://api.thingspeak.com/update?api_key=GGFTTH1UFCNYT3HU"
        if isinstance(event, MessageEvent):
            r=requests.get(laturl)
            d = r.json()
            lat = float(d['feeds'][-1]['field1'])
            r=requests.get(logurl)
            d = r.json()
            log = float(d['feeds'][-1]['field2'])
            if(event.message.text == "腳踏車位置"):
                location_message = LocationSendMessage(
                    title='離你最近的腳車',
                    address='位置',
                    latitude=lat,
                    longitude=log
                )
                line_bot_api.reply_message(
                    event.reply_token,
                    location_message
                )
            elif(event.message.text == "解鎖"):
                r=requests.get(lckurl)
                d = r.json()
                if(d['feeds'][-1]['field3'] == "0"):
                    uurl = uurl + "&amp;field1=" + str(lat) + "&amp;field2=" + str(log) + "&amp;field3=1"
                    requests.post(uurl)
                    line_bot_api.reply_message(
                        event.reply_token,
                        TextSendMessage(text="解鎖成功")
                    )
                
                elif(d['feeds'][-1]['field3'] == "1"):
                    line_bot_api.reply_message(
                        event.reply_token,
                        TextSendMessage(text="已有人租借")
                    )
            elif(event.message.text == "還車"):
                uurl = uurl + "&amp;field1=" + str(lat) + "&amp;field2=" + str(log) + "&amp;field3=0"
                requests.post(uurl)
                line_bot_api.reply_message(
                    event.reply_token,
                    TextSendMessage(text="還車成功")
                )
    return 'OK'

if __name__ == "__main__":
    app.run()

