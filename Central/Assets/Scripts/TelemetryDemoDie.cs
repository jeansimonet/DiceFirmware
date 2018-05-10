﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class TelemetryDemoDie : MonoBehaviour
{
    public TelemetryDie graphs;
    public RawImage dieImage;
    public Die3D die3D;


    private void Awake()
    {
    }
    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}

    public void Setup(Die die)
    {
        graphs.Setup(die.name);
        var rt = die3D.Setup(1);
        dieImage.texture = rt;
    }

    public void OnTelemetryReceived(Vector3 acc, int millis)
    {
        graphs.OnTelemetryReceived(acc, millis);
        die3D.UpdateAcceleration(acc);

    }
}
