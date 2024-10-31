package api

import (
	"Havoc/pkg/logger"
	"Havoc/pkg/utils"

	"encoding/json"
	"errors"
	"fmt"
	"github.com/gin-gonic/gin"
	"io"
	"net/http"
)

func (api *ServerApi) listenerStart(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		listener map[string]any
		name     string
		protocol string
		options  map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("got request on /api/listener/start: " + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		logger.DebugError("error parsing request: %v", err)
		goto ERROR
	}

	protocol, err = utils.MapKey[string](listener, "protocol")
	if err != nil {
		logger.DebugError("error parsing request: %v", err)
		goto ERROR
	}

	if _, ok := listener["config"]; ok {
		options, err = utils.MapKey[map[string]any](listener, "config")
		if err != nil {
			logger.DebugError("error parsing request: %v", err)
			goto ERROR
		}
	}

	if err = api.ListenerStart(name, protocol, options); err != nil {
		ctx.JSON(http.StatusInternalServerError, gin.H{
			"error": err.Error(),
		})
		return
	}

	ctx.AbortWithStatus(http.StatusOK)
	return

ERROR:
	ctx.AbortWithStatus(http.StatusInternalServerError)
}

func (api *ServerApi) listenerStop(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		name     string
		listener map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	logger.Debug("got request on /api/listener/stop:" + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		goto ERROR
	}

	if err = api.ListenerStop(name); err != nil {
		goto ERROR
	}

	ctx.AbortWithStatus(http.StatusOK)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerRestart(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		name     string
		listener map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	logger.Debug("got request on /api/listener/restart: " + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		goto ERROR
	}

	if err = api.ListenerRestart(name); err != nil {
		goto ERROR
	}

	ctx.AbortWithStatus(http.StatusOK)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerRemove(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		name     string
		listener map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	logger.Debug("got request on /api/listener/remove: " + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		goto ERROR
	}

	if err = api.ListenerRemove(name); err != nil {
		goto ERROR
	}

	ctx.AbortWithStatus(http.StatusOK)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerEdit(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		listener map[string]any
		config   map[string]any
		name     string
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the event data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("got request on /api/listener/edit: " + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		goto ERROR
	}

	config, err = utils.MapKey[map[string]any](listener, "config")
	if err != nil {
		goto ERROR
	}

	// process listener event
	if err = api.ListenerEdit(name, config); err != nil {
		goto ERROR
	}

	ctx.AbortWithStatus(http.StatusOK)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerEvent(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		event    map[string]any
		protocol string
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the event data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("got request on /api/listener/event:" + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &event); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	protocol, err = utils.MapKey[string](event, "protocol")
	if err != nil {
		goto ERROR
	}

	// process listener event
	if event, err = api.ListenerEvent(protocol, event); err != nil {
		goto ERROR
	}

	ctx.JSON(http.StatusOK, event)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerConfig(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		listener map[string]any
		config   map[string]any
		name     string
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the event data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("got request on /api/listener/config:" + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &listener); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	name, err = utils.MapKey[string](listener, "name")
	if err != nil {
		goto ERROR
	}

	// process listener event
	if config, err = api.ListenerConfig(name); err != nil {
		goto ERROR
	}

	ctx.JSON(http.StatusOK, config)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) listenerList(ctx *gin.Context) {
	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	logger.Debug("got request on /api/listener/list")

	ctx.JSON(http.StatusOK, api.ListenerList())
	return
}
